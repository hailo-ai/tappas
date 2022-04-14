# TAPPAS Framework

TAPPAS is a GStreamer based library of plug-ins. It enables using a Hailo devices within gstreamer pipelines to create inteliggent video processing applications.  

## What is GStreamer?

GStreamer is a framework for creating streaming media applications. ​

GStreamer's development framework makes it possible to write any type of streaming multimedia application. The GStreamer framework is designed to make it easy to write applications that handle audio or video or both. It isn't restricted to audio and video and can process any kind of data flow. ​The framework is based on plugins that will provide various codecs and other functionality. The plugins can be linked and arranged in a pipeline. This pipeline defines the flow of the data. ​The GStreamer core function is to provide a framework for plugins, data flow, and media type handling/negotiation. It also provides an API to write applications using the various plugins.​

For additional details check [GStreamer overview](terminology.md#gstreamer-framework)

## Hailo GStreamer Concepts

into the GStreamer framework, Hailo brings its functionality so we can infer video frames easily and intuitively without compromising on performance and flexibility.

### Hailo Concepts

- **Network encapsulation​** - Since in a configured network group, there are only input and output layers a GstHailoNet will be associated to a "Network" by its configured input and output pads​

- **Network independent elements​** - The GStreamer elements will be network independent, so the same infrastructure elements can be used for different applicative pipelines that use different NN functionality, configuration, activation, and pipelines​. Using the new API we can better decouple network configuration and activation stages and thus better support network switch​

- **GStreamer Hailo decoupling​** - Applicative code will use Hailo API and as such will be GStreamer independent. This will help us build and develop the NN and postprocessing functionality in a controlled environment (with all modern IDE and debugging capabilities).

- **Context control​** - Our elements will be contextless and thus leave the context (thread) control to the pipeline builder​

- **GStreamer reuse​** - our pipeline will use as many off the shelf GStreamer elements​

### Hailo GStreamer Elements

- [HailoNet](elements/hailo_net.md) - Element for sending and reciveing data from Hailo-8 chip
- [HailoFilter](elements/hailo_filter.md) - Element that enables the user to apply a postprocess or drawing operation to a frame and its tensors
- [HailoPython](elements/hailo_python.md) - Element that enables the user to apply a postprocess or drawing operation to a frame and its tensors via python.
- [HailoMuxer](elements/hailo_muxer.md) - Muxer element used for Multi-Hailo-8 setups
- [HailoDeviceStats](elements/hailo_device_stats.md) - Hailodevicestats is an element that samples power and temperature
- [HailoAggregator](elements/hailo_aggregator.md) - HailoAggregator is an element designed for applications with cascading networks. It has 2 sink pads and 1 source
- [HailoCropper](elements/hailo_cropper.md) - HailoCropper is an element designed for applications with cascading networks. It has 1 sink and 2 sources
- [HailoTileAggregator](elements/hailo_tile_aggregator.md) - HailoTileAggregator is an element designed for applications with tiles. It has 2 sink pads and 1 source
- [HailoTileCropper](elements/hailo_tile_cropper.md) - HailoCropper is an element designed for applications with tiles. It has 1 sink and 2 sources
- [HailoTracker](elements/hailo_tracker.md) - HailoTracker is an element that applies Joint Detection and Embedding (JDE) model with Kalman filtering to track object instances.
