all: PICOFoxweb

clean:
	@rm -rf *.o
	@rm -rf PICOFoxweb

CFLAGS += -lssl -lcrypto

PICOFoxweb: main.o httpd.o
	gcc -o PICOFoxweb $^ -lssl -lcrypto

main.o: main.c httpd.h
	gcc -c -o main.o main.c -Wall -Wextra

httpd.o: httpd.c httpd.h
	gcc -c -o httpd.o httpd.c -Wall -Wextra


install: PICOFoxweb
	useradd -c "PICOFoxweb project user" -r -s /sbin/nologin -d /var/www/PICO-Foxweb picouser
	mkdir -p /var/www/PICO-Foxweb
	cp -r ./webroot/* /var/www/PICO-Foxweb
	chown -R picouser:picouser /var/www/PICO-Foxweb
	install -o root -g root -m 0755 PICOFoxweb /usr/local/sbin/
	install -o root -g root -m 0644 pico-foxweb.service /etc/systemd/system/
	chown -R picouser:picouser /var/log

	mkdir -p /etc/PICO-Foxweb/certs
	cp -v ./certs/server.crt ./certs/server.key /etc/PICO-Foxweb/certs/
	chmod 444 /etc/PICO-Foxweb/certs/server.key
	chmod 444 /etc/PICO-Foxweb/certs/server.crt
	chown -R picouser:picouser /etc/PICO-Foxweb

	systemctl daemon-reload


uninstall:
	systemctl stop pico-foxweb.service
	rm -rf /var/www/PICO-Foxweb
	rm -f /usr/local/sbin/PICOFoxweb
	rm -f /etc/systemd/system/pico-foxweb.service
	rm -f /var/log/PICOFoxweb_log.log
	rm -rf /etc/PICO-Foxweb
	userdel -f picouser
	systemctl daemon-reload
