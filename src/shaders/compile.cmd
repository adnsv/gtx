glslangValidator --glsl-version 450 -V -x --vn polyline_frag -o tmp/polyline.frag.glsl450.h -S frag polyline-frag.glsl
glslangValidator --glsl-version 450 -V -x --vn polyline_vert -o tmp/polyline.vert.glsl450.h -S vert polyline-vert.glsl
glslangValidator --glsl-version 450 -V -x --vn polyline_geom -o tmp/polyline.geom.glsl450.h -S geom polyline-geom.glsl
