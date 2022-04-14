# Sanity pipeline

## Table of Contents

- [Sanity pipeline](#sanity-pipeline)
  - [Table of Contents](#table-of-contents)
  - [Overview](#overview)
    - [Sanity GStreamer](#sanity-gstreamer)

## Overview

Sanity apps purpose is to help you verify that all the required components have been installed successfully.

First of all, you would need to run `sanity_gstreamer.sh` and make sure that the image presented looks like the one that would be presented later.

### Sanity GStreamer

This app should launch first.
> *NOTE*: Open the source code in your preferred editor to see how simple this app is.

In order to run the app just `cd` to the `sanity_pipeline` directory and launch the app

```sh
cd $TAPPAS_WORKSPACE/apps/gstreamer/raspberrypi/sanity_pipeline
./sanity_gstreamer.sh
```

The output should look like:
<div align="center">
    <img src="readme_resources/sanity_gstreamer.png" width="300px" height="250px"/>
</div>

If the output is similar to the image shown above, you are good to go to the next verification phase!
