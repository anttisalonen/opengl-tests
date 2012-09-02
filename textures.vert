attribute vec3 a_Position;
attribute vec2 a_Texcoord;

uniform mat4 u_MVP;

varying vec2 v_Texcoord;

void main()
{
    gl_Position = u_MVP * vec4(a_Position, 1.0);
    v_Texcoord = a_Texcoord;
}

