# AWS IoT Certificates

Place firmware-side certificates in this folder when you are ready to connect
the ESP32 directly to AWS IoT Core.

Expected files:

- `AmazonRootCA1.pem`
- `device.pem.crt`
- `private.pem.key`

Do not commit private keys to Git.

This is the single certificate location used for direct ESP32 -> AWS IoT Core
connection.
