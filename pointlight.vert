attribute vec3 a_Position;
attribute vec2 a_texCoord;
attribute vec3 a_Normal;

uniform mat4 u_MVP;
uniform vec3 u_pointLightPosition;

varying vec2 v_texCoord;
varying vec3 v_Normal;
varying float v_PointLightDistance;

void main()
{
    gl_Position = u_MVP * vec4(a_Position, 1.0);
    v_texCoord = a_texCoord;
    v_Normal = a_Normal;
    v_PointLightDistance = distance(a_Position, u_pointLightPosition);
}

