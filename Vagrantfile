#e -*- mode: ruby -*-
# vi: set ft=ruby :

# This will setup a clean Ubuntu1404 LTS env with a python virtualenv called "pyre" for testing

$script = <<SCRIPT
apt-get update
add-apt-repository ppa:fkrull/deadsnakes-python2.7
apt-get update
apt-get install -y python-pip python-dev git htop virtualenvwrapper python2.7 python-virtualenv python-support cython \
git-all build-essential libtool pkg-config autotools-dev autoconf automake cmake uuid-dev libpcre3-dev valgrind \
libffi-dev

# only execute this next line if interested in updating the man pages as well (adds to build time):
# sudo apt-get install -y asciidoc

cd /vagrant
if [ ! -d gs1 ]; then
    git clone https://github.com/imatix/gsl.git
fi
cd gsl/src
make
sudo make install

cd /vagrant
if [ ! -d zproject ]; then 
    git clone https://github.com/zeromq/zproject.git
fi
cd zproject
./autogen.sh
./configure
make
sudo make install

cd /vagrant
if [ ! -d libsodium ]; then
    git clone --depth 1 -b stable https://github.com/jedisct1/libsodium.git
fi
cd libsodium
./autogen.sh && ./configure && make && make check
sudo make install
sudo ldconfig
cd ..

if [ ! -d libzmq ]; then
    git clone git://github.com/zeromq/libzmq.git
fi
cd libzmq
./autogen.sh
# do not specify "--with-libsodium" if you prefer to use internal tweetnacl
# security implementation (recommended for development)
./configure --with-libsodium && make && make check
sudo make install
sudo ldconfig
cd ..

if [ ! -d czmq ]; then
    git clone git://github.com/zeromq/czmq.git
fi
cd czmq
./autogen.sh && ./configure && make && make check
sudo make install
sudo ldconfig
sudo python setup.py install
cd ..

cd /vagrant
./autogen.sh && ./configure && make && make check
SCRIPT

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"
VAGRANTFILE_LOCAL = 'Vagrantfile.local'

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  config.vm.box = 'ubuntu/trusty64'
  config.vm.provision "shell", inline: $script
  config.vm.network "public_network"

  config.vm.provider :virtualbox do |vb|
    vb.customize ["modifyvm", :id, "--cpus", "2", "--ioapic", "on", "--memory", "512" ]
  end

  if File.file?(VAGRANTFILE_LOCAL)
    external = File.read VAGRANTFILE_LOCAL
    eval external
  end
end
