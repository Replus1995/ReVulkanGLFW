/* Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ValkanCudaApp.h"


// convert floating point rgba color to 32-bit integer
__device__ unsigned int rgbaFloatToInt(float4 rgba) {
  rgba.x = __saturatef(rgba.x);  // clamp to [0.0, 1.0]
  rgba.y = __saturatef(rgba.y);
  rgba.z = __saturatef(rgba.z);
  rgba.w = __saturatef(rgba.w);
  return ((unsigned int)(rgba.w * 255.0f) << 24) |
         ((unsigned int)(rgba.z * 255.0f) << 16) |
         ((unsigned int)(rgba.y * 255.0f) << 8) |
         ((unsigned int)(rgba.x * 255.0f));
}

__device__ float4 rgbaIntToFloat(unsigned int c) {
  float4 rgba;
  rgba.x = (c & 0xff) * 0.003921568627f;          //  /255.0f;
  rgba.y = ((c >> 8) & 0xff) * 0.003921568627f;   //  /255.0f;
  rgba.z = ((c >> 16) & 0xff) * 0.003921568627f;  //  /255.0f;
  rgba.w = ((c >> 24) & 0xff) * 0.003921568627f;  //  /255.0f;
  return rgba;
}

// row pass using texture lookups
__global__ void d_boxfilter_rgba_x(cudaSurfaceObject_t* dstSurfMipMapArray,
                                   cudaTextureObject_t textureMipMapInput,
                                   size_t baseWidth, size_t baseHeight,
                                   size_t mipLevels, int filter_radius) {
  float scale = 1.0f / (float)((filter_radius << 1) + 1);
  unsigned int y = blockIdx.x * blockDim.x + threadIdx.x;

  if (y < baseHeight) {
    for (uint32_t mipLevelIdx = 0; mipLevelIdx < mipLevels; mipLevelIdx++) {
      uint32_t width =
          (baseWidth >> mipLevelIdx) ? (baseWidth >> mipLevelIdx) : 1;
      uint32_t height =
          (baseHeight >> mipLevelIdx) ? (baseHeight >> mipLevelIdx) : 1;
      if (y < height && filter_radius < width) {
        float px = 1.0 / width;
        float py = 1.0 / height;
        float4 t = make_float4(0.0f);
        for (int x = -filter_radius; x <= filter_radius; x++) {
          t += tex2DLod<float4>(textureMipMapInput, x * px, y * py,
                                (float)mipLevelIdx);
        }

        unsigned int dataB = rgbaFloatToInt(t * scale);
        surf2Dwrite(dataB, dstSurfMipMapArray[mipLevelIdx], 0, y);

        for (int x = 1; x < width; x++) {
          t += tex2DLod<float4>(textureMipMapInput, (x + filter_radius) * px,
                                y * py, (float)mipLevelIdx);
          t -=
              tex2DLod<float4>(textureMipMapInput, (x - filter_radius - 1) * px,
                               y * py, (float)mipLevelIdx);
          unsigned int dataB = rgbaFloatToInt(t * scale);
          surf2Dwrite(dataB, dstSurfMipMapArray[mipLevelIdx],
                      x * sizeof(uchar4), y);
        }
      }
    }
  }
}

// column pass using coalesced global memory reads
__global__ void d_boxfilter_rgba_y(cudaSurfaceObject_t* dstSurfMipMapArray,
                                   cudaSurfaceObject_t* srcSurfMipMapArray,
                                   size_t baseWidth, size_t baseHeight,
                                   size_t mipLevels, int filter_radius) {
  unsigned int x = blockIdx.x * blockDim.x + threadIdx.x;
  float scale = 1.0f / (float)((filter_radius << 1) + 1);

  for (uint32_t mipLevelIdx = 0; mipLevelIdx < mipLevels; mipLevelIdx++) {
    uint32_t width =
        (baseWidth >> mipLevelIdx) ? (baseWidth >> mipLevelIdx) : 1;
    uint32_t height =
        (baseHeight >> mipLevelIdx) ? (baseHeight >> mipLevelIdx) : 1;

    if (x < width && height > filter_radius) {
      float4 t;
      // do left edge
      int colInBytes = x * sizeof(uchar4);
      unsigned int pixFirst = surf2Dread<unsigned int>(
          srcSurfMipMapArray[mipLevelIdx], colInBytes, 0);
      t = rgbaIntToFloat(pixFirst) * filter_radius;

      for (int y = 0; (y < (filter_radius + 1)) && (y < height); y++) {
        unsigned int pix = surf2Dread<unsigned int>(
            srcSurfMipMapArray[mipLevelIdx], colInBytes, y);
        t += rgbaIntToFloat(pix);
      }

      unsigned int dataB = rgbaFloatToInt(t * scale);
      surf2Dwrite(dataB, dstSurfMipMapArray[mipLevelIdx], colInBytes, 0);

      for (int y = 1; (y < filter_radius + 1) && ((y + filter_radius) < height);
           y++) {
        unsigned int pix = surf2Dread<unsigned int>(
            srcSurfMipMapArray[mipLevelIdx], colInBytes, y + filter_radius);
        t += rgbaIntToFloat(pix);
        t -= rgbaIntToFloat(pixFirst);

        dataB = rgbaFloatToInt(t * scale);
        surf2Dwrite(dataB, dstSurfMipMapArray[mipLevelIdx], colInBytes, y);
      }

      // main loop
      for (int y = (filter_radius + 1); y < (height - filter_radius); y++) {
        unsigned int pix = surf2Dread<unsigned int>(
            srcSurfMipMapArray[mipLevelIdx], colInBytes, y + filter_radius);
        t += rgbaIntToFloat(pix);

        pix = surf2Dread<unsigned int>(srcSurfMipMapArray[mipLevelIdx],
                                       colInBytes, y - filter_radius - 1);
        t -= rgbaIntToFloat(pix);

        dataB = rgbaFloatToInt(t * scale);
        surf2Dwrite(dataB, dstSurfMipMapArray[mipLevelIdx], colInBytes, y);
      }

      // do right edge
      unsigned int pixLast = surf2Dread<unsigned int>(
          srcSurfMipMapArray[mipLevelIdx], colInBytes, height - 1);
      for (int y = height - filter_radius;
           (y < height) && ((y - filter_radius - 1) > 1); y++) {
        t += rgbaIntToFloat(pixLast);
        unsigned int pix = surf2Dread<unsigned int>(
            srcSurfMipMapArray[mipLevelIdx], colInBytes, y - filter_radius - 1);
        t -= rgbaIntToFloat(pix);
        dataB = rgbaFloatToInt(t * scale);
        surf2Dwrite(dataB, dstSurfMipMapArray[mipLevelIdx], colInBytes, y);
      }
    }
  }
}




void vulkanImageCUDA::cudaUpdateVkImage()
{
	cudaVkSemaphoreWait(cudaExtVkUpdateCudaSemaphore);

	int nthreads = 128;

	/*Perform 2D box filter on image using CUDA */
	d_boxfilter_rgba_x << <m_imageHeight / nthreads, nthreads, 0, streamToRun >> > (
		d_surfaceObjectListTemp, textureObjMipMapInput, m_imageWidth, m_imageHeight,
		mipLevels, filter_radius);

	d_boxfilter_rgba_y << <m_imageWidth / nthreads, nthreads, 0, streamToRun >> > (
		d_surfaceObjectList, d_surfaceObjectListTemp, m_imageWidth, m_imageHeight,
		mipLevels, filter_radius);

	varySigma();

	cudaVkSemaphoreSignal(cudaExtCudaUpdateVkSemaphore);
}
