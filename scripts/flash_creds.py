from enum import IntEnum
import struct

class CredType(IntEnum):
	CRED_HTTPS_SERVER_PRIVATE_KEY = 0
	CRED_HTTPS_SERVER_CERTIFICATE = 1

	CRED_HTTPS_CLIENT_PRIVATE_CERTIFICATE = 2
	CRED_HTTPS_CLIENT_CA = 3

	CRED_AWS_PRIVATE_KEY = 4
	CRED_AWS_CERTIFICATE = 5
	CRED_AWS_ROOT_CA = 6

