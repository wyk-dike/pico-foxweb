#!/bin/bash
mkdir -p ./certs

openssl req -x509 -newkey rsa:4096 -sha256 -days 365 -nodes \
    -keyout certs/server.key -out certs/server.crt \
    -subj "/CN=localhost" -addext "subjectAltName=DNS:localhost,IP:127.0.0.1"

echo -e "=============================="
openssl x509 -in certs/server.crt -text -noout

sudo cp certs/server.crt /usr/local/share/ca-certificates/
sudo update-ca-certificates