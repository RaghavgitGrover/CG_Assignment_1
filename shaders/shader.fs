#version 330

in vec2 TexCoord;
out vec4 diffuseColor;
uniform vec4 uColor;

void main()
{
    // For both cups and marbles, use a circular mask.
    vec2 pos = TexCoord * 2.0 - vec2(1.0);
    float r2 = dot(pos, pos);

    // For marbles, we want a smooth, lit look.
    // If r2 is greater than 1.0, discard (outside circle).
    if(r2 > 1.0) discard;

    // Compute a pseudo-normal for a sphere based on the 2D coordinate.
    float z = sqrt(max(0.0, 1.0 - r2));
    vec3 normal = normalize(vec3(pos, z));
    
    // Simple Phong lighting:
    vec3 lightDir = normalize(vec3(0.5, 0.5, 1.0));
    vec3 viewDir = normalize(vec3(0.0, 0.0, 1.0));
    float ambient = 0.3;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    float specularStrength = 0.5;
    float lighting = ambient + diff + specularStrength * spec;
    diffuseColor = vec4(uColor.rgb * lighting, uColor.a);
}
