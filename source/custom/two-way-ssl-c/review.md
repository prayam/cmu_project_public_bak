# connection confirmed

./ssl-test server 8888 ./keys/ca/ca.crt ./keys/server/server.crt ./keys/server/server.key

./ssl-test client localhost:8888 ./keys/ca/ca.crt ./keys/client/client.crt ./keys/client/client.key


## mTLS를 위한 키 설정을 아래를 따르도록 한다.
https://codeburst.io/mutual-tls-authentication-mtls-de-mystified-11fa2a52e9cf

## 상호인증 server / client 코드는 git 소스를 참조하고,  다른 예제 코드를 비교 리뷰하여 교차검증하였다
https://github.com/zapstar/two-way-ssl-c
https://aticleworld.com/ssl-server-client-using-openssl-in-c/
Openssl wiki
https://wiki.openssl.org/index.php/Simple_TLS_Server
https://wiki.openssl.org/index.php/SSL/TLS_Client


## OpenSSL Build
1.1.0 이후에는 make depend 를 하지 않아도 됨.  

$ ./config \
--prefix=/opt/openssl \
--openssldir=/opt/openssl \
no-shared \
-DOPENSSL_TLS_SECURITY_LEVEL=2 \
enable-ec_nistp_64_gcc_128

- 확인하고, 다음 옵션을 켜도록 하자 
- enable-ec_nistp_64_gcc_128	Use on little endian platforms when GCC supports __uint128_t. ECDH is about 2 to 4 times faster. Not enabled by default because Configure can't determine it. Enable it if your compiler defines __SIZEOF_INT128__, the CPU is little endian and it tolerates unaligned data access.


$ make
$ make test
$ sudo make install
