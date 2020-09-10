Vagrant.configure(2) do |config|
    # Setup VirtualBox Provider
    config.vm.box = "bento/ubuntu-20.04"

    config.vm.box_check_update = false
    config.vm.provider "virtualbox" do |vb|
        vb.gui = false
    end

    config.vm.provision "shell", inline: "sudo apt-get update"
    config.vm.provision "shell", inline: "sudo apt-get install -y build-essential automake libtool cmake pkg-config gdb libsystemd-dev libexpat1-dev"

    config.vm.provision "shell", inline: "sudo groupadd service-client || true"
    config.vm.provision "shell", inline: "sudo useradd service-user -g service-client || true"

    config.vm.provision "shell", path: "install-sdbus.sh"
end
