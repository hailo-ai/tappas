# Install

Currently, the method for installing offline is using our [pre-built Docker image](installation/docker-install.md)

## View the documentation offline

TAPPAS documentation is written in `markdown` format, there are plenty of `markdown` offline viewers:

### VSCode

VSCode has out of the box support for Markdown files - [Markdown Preview](https://code.visualstudio.com/docs/languages/markdown#_markdown-preview)

### Grip

> the below describes local setup for linux, but it can be adapted for other OSs.

Render local markdown files

```sh
# firstly, begin by installing `grip`
pip3 install grip

# CD into tappas home folder and launch the grip server
cd tappas
sudo env "PATH=$PATH" grip 80
```

Open your preferred browser and enter the following url:  `http://localhost:80`
