wimg median
===========
This is a "weighted" median filter. This means that pixels in different parts
of the median kernel have different sizes on the X axis of the sorted pixel
values, when intensity is mapped as Y and X moves from pixel to pixel. You read
that right. More-central pixels take up more space in the list.

But... Why?
===========
You will find that it results in much sharper images than using a 3x3 flat
median kernel, yet still having the property of being centered. Also, you can
run it recursively without blowing up all edges. You can also "blur" the result
(to hide kernel jaggies) in a much more powerful way than you can with a flat
kernel.

I still don't understand how this works.
========================================
Read the source code. I did my best to keep things clean.

Got any examples?
=================
Sure thing.

Original:
- http://i.imgur.com/yRMe6vw.png
Gimp, 1px radias "Despeckle", -1 black, 256 white, not adaptive, not recursive (normal 3x3 median filter):
- http://i.imgur.com/AwhwsSr.png
Above, at 50% wetness (blend with original):
- http://i.imgur.com/tD6PIhU.png
Wimg median, normal:
- http://i.imgur.com/HFdwiXf.png
Wimg median, blurry:
- http://i.imgur.com/O2Lx1fs.png
Wimg median, blurrier:
- http://i.imgur.com/mIzMm7w.png

Yes, "normal", "blurry", and "blurrier" look extremely similar in real world
examples. However, you will find that "blurrier" shows slightly less aliasing
than the other two. The difference between "normal" and "blurry", on the other
hand, is invisible except for in certain pathological pixel patterns.

"Normal" has a little blur where gimp doesn't.
==============================================
This is because the method I use gives 16 samples to calculate a median of. The
"correct" way to select a median between an even number of elements is to
average the central two samples. If you dislike this, you can patch the method
and add an extra center sample. Don't remove one; you'll crash on one-dimension
images. Being biased towards the higher or lower center sample will result in
even more bright or dark fringing on certain edge patterns.

There's a little aliasing.
======================
That's a drawback of denoising with a median filter. Certain images, including
this one, are anti-aliased very sharply, but also have ringing. Medians share
properties with surface blur type effects, and when you filter over these sharp
ringed edges, you get the same aliasing behavior. The "blurrier" variant should
cause less of this behavior, though.

It's not good enough, and I can't code. What do I do?
=====================================================
Upscale your image to 300% with whatever algorithm you want. Now run your
normal flat median with a large kernel (2px radius? 3px radius?) -- you will
get less of the dot pattern outline effect than if you ran the median on the
native resolution of the image. Now downscale to the original resolution.

I don't want to do that.
========================
Use waifu2x's jpeg denoising. It's not general purpose, but it might work.