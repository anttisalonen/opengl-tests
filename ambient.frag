varying vec2 v_Texcoord;

uniform sampler2D s_texture;
uniform vec4 u_ambientLight;

void main()
{
    gl_FragColor = texture2D(s_texture, v_Texcoord) * u_ambientLight;
}

