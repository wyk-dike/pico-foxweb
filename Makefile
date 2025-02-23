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


BASE_PATH=/APP/PICOFoxweb
INSTALL_PATH=$(BASE_PATH)/bin
SERVICE_PATH=/etc/systemd/system
WEBROOT_PATH=$(BASE_PATH)/webroot

install: PICOFoxweb
	mkdir -p $(INSTALL_PATH)
	install -o root -g root -m 755 PICOFoxweb $(INSTALL_PATH)/

	sed -i 's|^ExecStart=.*|ExecStart=$(INSTALL_PATH)/PICOFoxweb 8080|' pico-foxweb.service
	install -o root -g root -m 644 pico-foxweb.service $(SERVICE_PATH)/
	systemctl daemon-reload

	mkdir -p $(WEBROOT_PATH)
	cp -r webroot/* $(WEBROOT_PATH)/
	chown -R root:root $(WEBROOT_PATH)
	chmod -R 755 $(WEBROOT_PATH)

uninstall:
	systemctl stop pico-foxweb.service
	rm -rf $(BASE_PATH)
	rm -f $(SERVICE_PATH)/pico-foxweb.service
	systemctl daemon-reload
