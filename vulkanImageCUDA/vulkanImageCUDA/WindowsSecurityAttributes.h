#pragma once


#include <windows.h>
#include <stdlib.h>
#include <assert.h>
#include <aclapi.h>

class WindowsSecurityAttributes {
protected:
	SECURITY_ATTRIBUTES m_winSecurityAttributes;
	PSECURITY_DESCRIPTOR m_winPSecurityDescriptor;

public:
	WindowsSecurityAttributes() {
		m_winPSecurityDescriptor = (PSECURITY_DESCRIPTOR)calloc(
			1, SECURITY_DESCRIPTOR_MIN_LENGTH + 2 * sizeof(void **));
		assert(m_winPSecurityDescriptor != (PSECURITY_DESCRIPTOR)NULL);

		PSID *ppSID = (PSID *)((PBYTE)m_winPSecurityDescriptor +
			SECURITY_DESCRIPTOR_MIN_LENGTH);
		PACL *ppACL = (PACL *)((PBYTE)ppSID + sizeof(PSID *));

		InitializeSecurityDescriptor(m_winPSecurityDescriptor,
			SECURITY_DESCRIPTOR_REVISION);

		SID_IDENTIFIER_AUTHORITY sidIdentifierAuthority =
			SECURITY_WORLD_SID_AUTHORITY;
		AllocateAndInitializeSid(&sidIdentifierAuthority, 1, SECURITY_WORLD_RID, 0, 0,
			0, 0, 0, 0, 0, ppSID);

		EXPLICIT_ACCESS explicitAccess;
		ZeroMemory(&explicitAccess, sizeof(EXPLICIT_ACCESS));
		explicitAccess.grfAccessPermissions =
			STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
		explicitAccess.grfAccessMode = SET_ACCESS;
		explicitAccess.grfInheritance = INHERIT_ONLY;
		explicitAccess.Trustee.TrusteeForm = TRUSTEE_IS_SID;
		explicitAccess.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
		explicitAccess.Trustee.ptstrName = (LPTSTR)*ppSID;

		SetEntriesInAcl(1, &explicitAccess, NULL, ppACL);

		SetSecurityDescriptorDacl(m_winPSecurityDescriptor, TRUE, *ppACL, FALSE);

		m_winSecurityAttributes.nLength = sizeof(m_winSecurityAttributes);
		m_winSecurityAttributes.lpSecurityDescriptor = m_winPSecurityDescriptor;
		m_winSecurityAttributes.bInheritHandle = TRUE;
	};
	~WindowsSecurityAttributes() {
		PSID *ppSID = (PSID *)((PBYTE)m_winPSecurityDescriptor +
			SECURITY_DESCRIPTOR_MIN_LENGTH);
		PACL *ppACL = (PACL *)((PBYTE)ppSID + sizeof(PSID *));

		if (*ppSID) {
			FreeSid(*ppSID);
		}
		if (*ppACL) {
			LocalFree(*ppACL);
		}
		free(m_winPSecurityDescriptor);
	};
	SECURITY_ATTRIBUTES *operator&() {
		return &m_winSecurityAttributes;
	};
};

