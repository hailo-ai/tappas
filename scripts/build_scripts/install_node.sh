#!/bin/bash
set -e

function install_node() {
    sudo apt-get install -y curl
    curl -sL https://deb.nodesource.com/setup_18.x | sudo -E bash -

    sudo apt-get install -y nodejs
    sudo apt-get install -y gcc g++ make
}
function install_yarn() {
    curl -sL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor | sudo tee /usr/share/keyrings/yarnkey.gpg >/dev/null
    echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" | sudo tee /etc/apt/sources.list.d/yarn.list
    sudo apt-get update && sudo apt-get install yarn
}

function install_el_tappas_requirements() {
    sudo apt-get install -y libnss3-dev libgdk-pixbuf2.0-dev libgtk-3-dev libxss-dev libigdgmm11

    pushd "${TAPPAS_WORKSPACE}/tools/el_tappas"
    yarn
    popd
}

function main() {
    install_node
    install_yarn
    install_el_tappas_requirements
}

git clone git@bitbucket.org:hailotech/el-tappas.git "${TAPPAS_WORKSPACE}/tools/el_tappas"
main
