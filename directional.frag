varying vec2 v_texCoord;
varying vec3 v_Normal;

uniform sampler2D s_texture;
uniform vec3 u_ambientLight;
uniform vec3 u_directionalLightDirection;
uniform vec3 u_directionalLightColor;

void main()
{
    vec4 light = vec4(u_ambientLight, 1.0);
    float directionalFactor = dot(normalize(v_Normal), -u_directionalLightDirection);
    vec4 directionalLight;

    if(directionalFactor > 0.0)
        directionalLight = vec4(u_directionalLightColor, 1.0) * directionalFactor;
    else
        directionalLight = vec4(0.0);

    light += directionalLight;
    light = clamp(light, 0, 1);
    gl_FragColor = texture2D(s_texture, v_texCoord) * light;
}

