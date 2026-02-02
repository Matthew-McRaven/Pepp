Every year or so, one of my certs expires.
Here is how you should assign new secrets in this repo.

You only reeally need to update: MAC_BUILD_CERTIFICATE_BASE64, but I include instructions for all fields for completness.

- Create a Developer ID Application on: https://developer.apple.com/account/resources/certificates
  This will read something like: Developer ID Application: Matthew McRaven  (<redacted>)
  Download this cert, add it to your keychain.
- Export that cert as P12.
  Look at the Developer ID Applicatio password note field (bitwarden)
  This password is MAC_P12_PASSWORD
- The redacted bit is MAC_DEVELOPER_TEAM_ID
- Base64 encode the cert.
  This is MAC_BUILD_CERTIFICATE_BASE64
- Ensure you have a `notarytool` password on https://account.apple.com/account/manage
  This is MAC_NOTARYTOOL_PASSWORD
- MAC_DEVELOPER_NAME is just my name, MAC_APPLE_ID is my gmail email.
- MAC_KEYCHAIN_PASSWORD is some random, secure password.
  Since I've place my dev cert on this machine, this password better be secure.
  Otherwise people could sign code with my name if the compromise the runner.
