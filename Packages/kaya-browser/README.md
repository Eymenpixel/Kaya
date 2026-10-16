# Kaya Browser Launcher

Open a web page in a browser already installed on the Linux host:

- `kaya-browser https://example.com` chooses Chrome, Chromium, then Firefox.
- `kaya-browser --prefer firefox https://example.com` chooses one browser.
- `kaya-browser --list` shows which supported browsers are available.

This package is a launcher, not a browser distribution. It does not download
or bundle browser binaries. In WSL, a graphical browser also needs WSLg or
another configured display server.
