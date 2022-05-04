FROM irv-development

# Install dependencies
RUN sudo apt-get update && \
    sudo apt-get install -y software-properties-common

# Install a never neovim
RUN sudo add-apt-repository ppa:neovim-ppa/stable && \
    sudo apt-get update && \
    sudo apt-get install -y neovim clangd ranger

# Download my dotfiles (add is used to force rebuild after a new commit)
ADD "https://api.github.com/repos/iiKoe/dotfiles/commits?per_page=1" .latest_dotfile_commit
RUN git clone https://github.com/iiKoe/dotfiles.git

# Configure neovim
RUN mkdir ~/.config && ln -s ~/dotfiles/nvim ~/.config/nvim

# Force Packer to install all the neovim plugins
RUN nvim --headless -c ':q' > /dev/null 2>&1; exit 0 # first run is to load Packer
RUN nvim --headless -c 'autocmd User PackerComplete quitall' -c 'PackerSync' \
    > /dev/null 2>&1; exit 0

# Ranger navigation
RUN echo "alias r=\"source ranger\"" >> ~/.bashrc
