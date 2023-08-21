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
- [x] show profiling results in ImGui view

## Priority 2
- [ ] separate out the sky init + rendering code into something like r_sky.cpp
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
- [x] don't wait for idle at the end of each frame
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
- [ ] if you compile the converter, then make an edit in ImageLib, but not in the converter, it seems like the converter uses the new code, but you can no longer breakpoint in ImageLib. It will claim it's a stale cpp file
- [ ] Switch to new parallel mechanism in part, since it doesn't seem like you can use a pragma omp, inside of another one. Only the outter one applies