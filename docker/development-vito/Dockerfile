FROM irv-development

# Install dependencies
RUN sudo apt-get update && \
    sudo apt-get install -y software-properties-common

# Install a never neovim
RUN sudo add-apt-repository ppa:neovim-ppa/stable && \
    sudo apt-get update && \
    sudo apt-get install -y neovim clangd

# Download my dotfiles
RUN git clone https://github.com/iiKoe/dotfiles.git

# Configure neovim
RUN mkdir ~/.config && ln -s ~/dotfiles/nvim ~/.config/nvim

# Force Packer to install all the neovim plugins
RUN nvim --headless -c ':q' > /dev/null 2>&1; exit 0 # first run is to load Packer
RUN nvim --headless -c 'autocmd User PackerComplete quitall' -c 'PackerSync' \
    > /dev/null 2>&1; exit 0
