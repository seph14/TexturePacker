AtlasGenerator
==============
author: Seph Li<br>
http://solid-jellyfish.com

A tool packaging textures into single/multi atlas(es) and generate a Xml file saving all individual textures' name, position, size.

This software is supposed to be used with the Cinder C++ framework (https://github.com/cinder/Cinder).
The core algorithm is using Paul.Houx's(https://github.com/paulhoux) bin packaging code (https://github.com/cinder/Cinder/pull/362)

Functions:<br>
1. choose packaging method Single/Multi;<br>
2. change atlas size ( non power of 2 numbers will be floor down to a power of 2 number );<br>
3. load a single texture or load all textures in a folder;<br>
4. upon rendering, package will be resized to fit into the window (512x512);<br>
5. export - a filename with an extension (if no extension image will be saved as bmp) need to specified,
   and 2 xml files storing position, name, size of each texture will be saved. One is of absolute numbers and another one    is of ratios relative to the atlas size. An option to export flipped texture make it easy to use with other cinder   
   applications.<br>

Known issue:<br>
1. load a folder of textures in multi bin mode after clearing all textures will cause a rendering issue.<br>

Todos:<br>
1. ciUI block is actually out-of-date, so there might be more bugs related to the UI lurking somewhere, need to either improve it or implement another UI.
