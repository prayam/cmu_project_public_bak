#!/bin/sh

mkdir ca
mkdir client
mkdir server

cd ca
echo "\n>> create ca private key : passphrase setting required..."
openssl genpkey -out myca.key -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -aes-256-cbc
openssl ec -in myca.key -out ca.key

# don't need public key extraction
# echo "\n>> create ca public key from private key"
# openssl pkey -in ca.key -pubout -out ca-public.key

# echo "create ca csr"
# openssl req -new -key ca.key -out ca.csr

# echo "create ca crt"
# openssl x509 -req -days 365 -in ca.csr -signkey ca.key -out ca.crt

echo "\n>> create ca certificates"
openssl req -new -x509 -days 365 -key ca.key -out ca.crt \
-subj "/C=KR/L=Seoul/O=LGE/CN=ca.security.lge"

echo "\n>> examining ca certificates"
openssl x509 -text -in ca.crt -noout

cd ../server
echo "\n>> create server private key : passphrase setting required..."
openssl genpkey -out server.key -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -aes-256-cbc
openssl ec -in server.key -out server.key

# don't need public key extraction
# echo "\n>> create server public key from private key"
# openssl pkey -in server.key -pubout -out server-public.key

echo "\n>> create server csr"
openssl req -new -key server.key -out server.csr \
-subj "/C=KR/L=Seoul/O=LGE/CN=face.recog.server.Jetson"

echo "\n>> create server crt in the PEM format"
openssl x509 -req -in server.csr -CA ../ca/ca.crt -CAkey ../ca/ca.key \
-CAcreateserial -days 365 -out server.crt

echo "\n>> examining server cert"
openssl x509 -in server.crt -text -noout
rm server.csr

cd ../client
echo "\n>> create client private key : passphrase setting required..."
openssl genpkey -out client.key -algorithm EC -pkeyopt ec_paramgen_curve:P-256 -aes-256-cbc
openssl ec -in client.key -out client.key

# don't need public key extraction
# echo "\n>> create client public key from private key"
# openssl pkey -in client.key -pubout -out client-public.key

echo "\n>> create client csr"
openssl req -new -key client.key -out client.csr \
-subj "/C=KR/L=Seoul/O=LGE/CN=face.recog.client.PC"

echo "\n>> create client crt in the PEM format"
openssl x509 -req -in client.csr -CA ../ca/ca.crt -CAkey ../ca/ca.key \
-CAcreateserial -days 365 -out client.crt

echo "\n>> examining client cert"
openssl x509 -in client.crt -text -noout
rm client.csr

cd ../ca
mv myca.key ca.key
cd ../
