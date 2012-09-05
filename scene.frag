varying vec2 v_texCoord;
varying vec3 v_Normal;
varying float v_PointLightDistance;

uniform sampler2D s_texture;
uniform vec3 u_ambientLight;
uniform vec3 u_directionalLightDirection;
uniform vec3 u_directionalLightColor;
uniform vec3 u_pointLightColor;
uniform vec3 u_pointLightAttenuation;
uniform bool u_ambientLightEnabled;
uniform bool u_directionalLightEnabled;
uniform bool u_pointLightEnabled;

void main()
{
    vec4 light;
    float directionalFactor;
    vec4 directionalLight;
    vec4 pointLight;
    float pointLightFactor;

    light = vec4(0.0);

    if(u_ambientLightEnabled)
        light = vec4(u_ambientLight, 1.0);

    if(u_directionalLightEnabled) {
        directionalFactor = dot(normalize(v_Normal), -u_directionalLightDirection);
        if(directionalFactor > 0.0)
            directionalLight = vec4(u_directionalLightColor, 1.0) * directionalFactor;
        else
            directionalLight = vec4(0.0);

        light += directionalLight;
    }

    if(u_pointLightEnabled) {
        pointLightFactor = 1.0 / (u_pointLightAttenuation.x + u_pointLightAttenuation.y * v_PointLightDistance +
                    u_pointLightAttenuation.z * v_PointLightDistance * v_PointLightDistance);
        pointLightFactor = clamp(pointLightFactor, 0, 1);
        pointLight = vec4(pointLightFactor * u_pointLightColor, 1.0);
        light += pointLight;
    }

    light = clamp(light, 0, 1);
    gl_FragColor = texture2D(s_texture, v_texCoord) * light;
}

