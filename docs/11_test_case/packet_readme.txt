192.168.0.228:5000 >> server
192.168.0.217 >> client

1. login fail
	TCP Connection
		Packet No. 197 > SYN
		Packet No. 198 > SYN, ACK
		Packet No. 199 > ACK
		
	TCP Disconnection
		Packet No. 228 > RST

2. login success
	TCP Connection (Control Data port - 8092)
		Packet No. 244 > SYN
		Packet No. 245 > SYN, ACK
		Packet No. 246 > ACK
		
	TCP Connection (Secure Photo port - 8094)
		Packet No. 266 > SYN
		Packet No. 267 > SYN, ACK
		Packet No. 268 > ACK
		
	TCP Connection (NonSecure Photo port - 8096)
		Packet No. 274 > SYN
		Packet No. 275 > SYN, ACK
		Packet No. 276 > ACK
		
	TCP Connection (Meta Data port - 8098)
		Packet No. 282 > SYN
		Packet No. 285 > SYN, ACK
		Packet No. 286 > ACK
		
	frame matches "admin" # cannot find admin string. it's id. so we guess that the tls is working well

	
3. change secure > nonsecure mode
		Packet No. 7336 is the last packet received data through 8094 
	the mode was changed from secure to non-secure
		Packet No. 7337 is the first packet received data through 8096

4. logout
	TCP Disconnection (8094 port)
		Packet No. 14750 > FIN, ACK
	TCP Disconnection (8098 port)
		Packet No. 14758 > FIN, ACK
	TCP Disconnection (8096 port)
		Packet No. 14788 > FIN, ACK
	TCP Disconnection (8092 port)
		Packet No. 14791 > RST