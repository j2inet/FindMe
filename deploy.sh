sudo cp ./net.j2i.findMe /usr/local/bin
sudo cp ./net.j2i.findMe.service /etc/systemd/system/net.j2i.findMe.service
sudo systemctl daemon-reload
systemctl status net.j2i.findMe
systemctl status enable net.j2i.findMe
systemctl status start net.j2i.findMe
