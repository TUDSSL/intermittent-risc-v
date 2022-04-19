FROM irv-development

# Install dependencies
RUN sudo apt-get update && \
    sudo apt-get install -y software-properties-common

# Download my dotfiles
RUN git clone https://github.com/souravmohapatra/my-dotfiles.git

# Configure
RUN sed -i.bak -e '5,6d' ~/my-dotfiles/configurations/home/mohapatr/.bashrc
RUN cp ~/my-dotfiles/configurations/home/mohapatr/.bashrc ~/.bashrc
