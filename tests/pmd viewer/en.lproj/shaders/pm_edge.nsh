@uniform edge_offset "edge offset"=0.01,0.01,0.01,0.0
@uniform edge_color "edge color"=0.0,0.0,0.0,1.0

@vertex
uniform vec4 edge_offset;
void main() { gl_Position=gl_ModelViewProjectionMatrix*vec4(gl_Vertex.xyz+gl_Normal*edge_offset.xyz,gl_Vertex.w); }

@fragment
uniform vec4 edge_color;
void main() { gl_FragColor=edge_color; }
