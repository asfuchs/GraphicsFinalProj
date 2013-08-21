varying vec3 vLightVector;
varying vec3 vNormal;
varying vec3 vEye;

uniform vec3 uColor;
uniform float uShiny;
uniform vec3 uSpecular;

uniform sampler2D uTexUnit;
varying vec2 vTexCoord;

void main()
{
    vec3 lightColor = vec3(1, 1, 1);

    vec4 texColor1 = texture2D(uTexUnit, vTexCoord);

    vec3 AmbientColor = vec3(0.01, 0.01, 0.01) * lightColor;
    vec3 DiffuseColor = uColor * clamp(dot(normalize(vNormal), normalize(vLightVector)), 
    0.0, 1.0) * lightColor;
    
    vec3 reflection = reflect(normalize(-vLightVector), normalize(vNormal));
    
  
    vec3 specular = uSpecular * pow(clamp(dot(normalize(reflection), normalize(vEye)), 
                    0.0, 1.0), uShiny) * lightColor;
    
    gl_FragColor = vec4(vTexCoord.s, vTexCoord.t, 0, 1);
    gl_FragColor = vec4(AmbientColor + DiffuseColor + specular, 1.0);
    //gl_FragColor = vec4(abs(normalize(vNormal)), 1.0); // Show Normals
}

