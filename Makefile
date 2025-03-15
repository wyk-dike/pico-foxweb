all: PICOFoxweb

clean:
	@rm -rf *.o
	@rm -rf PICOFoxweb

PICOFoxweb: main.o httpd.o
	gcc -o PICOFoxweb $^

main.o: main.c httpd.h
	gcc -c -o main.o main.c

httpd.o: httpd.c httpd.h
	gcc -c -o httpd.o httpd.c


install: PICOFoxweb
	useradd -c "PICOFoxweb project user" -r -s /sbin/nologin -d /var/www/PICO-Foxweb picouser
	mkdir -p /var/www/PICO-Foxweb
	cp -r ./webroot/* /var/www/PICO-Foxweb
	chown -R picouser:picouser /var/www/PICO-Foxweb
	install -o root -g root -m 0755 PICOFoxweb /usr/local/sbin/
	install -o root -g root -m 0644 pico-foxweb.service /etc/systemd/system/
	chown -R picouser:picouser /var/log
	systemctl daemon-reload


uninstall:
	systemctl stop pico-foxweb.service
	rm -rf /var/www/PICO-Foxweb
	rm -f /usr/local/sbin/PICOFoxweb
	rm -f /etc/systemd/system/pico-foxweb.service
	rm -f /var/log/PICOFoxweb_log.log
	userdel -f picouser
	systemctl daemon-reload
