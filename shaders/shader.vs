#version 330

in vec3 Position;
uniform mat4 gWorld;
out vec2 TexCoord;

void main()
{
    gl_Position = gWorld * vec4(Position, 1.0);
    TexCoord = Position.xy + vec2(0.5);
}
