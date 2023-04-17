# TODO

## Priority 1
- [x] add functionaility for removing layouts
- [ ] Add support for running sponza again. Main menu of sorts -> scene selection?
- [ ] ui mouse events lua callbacks
- [ ] ui keyboard lua callbacks
- [x] shader include cache
- [x] multithread the converter
- [x] change profiling to be all in RAM, no file writing. Show min + max + avg
- [x] Make asset filenames consistent (all from PG_ASSET_DIR, parses only store relative path. Load() makes absolute path, or detects it at least)
- [ ] Switch to dynamic rendering
- [ ] add meshoptimizer back in
- [ ] Get remote console up and running again, and able to send commands (windows and linux)
- [ ] show profiling results in ImGui view

## Priority 2
- [ ] see if spirv optimizer is working again
- [ ] LZ4 compress converted assets + fastfiles
- [x] Get albedo + metalness image working for texturesets
- [ ] Get normal + roughness image working for texturesets
- [x] remove 2nd hash number in gfxImage cache composite names
- [ ] Make model exporter work with texturesets
- [ ] If assets are already in required/dependent fastfiles, dont put them in the top level FFs
- [ ] include both debug + non-debug shaders in FF, allow for runtime switching
- [ ] fix shader preproc options (allow from command line)
- [ ] mychanges for scripts
- [ ] mychanges for models
- [x] mychanges for ui layouts
- [ ] mychanges for materials 
- [ ] mychanges for shaders 
- [ ] mychanges for images/texturesets
- [ ] asset hashes for mychanges
- [ ] don't wait for idle at the end of each frame
- [ ] don't wait for idle while uploading images. Use barriers/whatever
- [ ] controller support
- [ ] allow for resizing window/multiple resolutions

## Priority 3
- [ ] add TIFF loading and saving
- [ ] Make sure image clamping vs wraping working correctly
- [ ] decouple graphics code from all assets (GfxImage) just store a handle, and remove Vulkan dependencies from Projects that dont need it anymore
- [ ] switch/add to .pmodel binary format, with binary <-> text tool
- [ ] add .pmodel -> .obj tool
- [ ] simplify TransitionImageLayoutImmediate args with defaults
- [ ] get VS to find spirv/vulkan/shaderc files
- [ ] investigate different mipmap filters
- [ ] do a pass on the code style consistency
- [ ] have the game window open at the location it was the last time the game ran
- [ ] allow for fixed aspect ratio
