#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_NormalMatrix; // 用于变换法线
uniform vec3 u_ViewPos;

out vec3 FragPos;
out vec3 Normal;
out vec3 ViewDir;

void main()
{
    FragPos = vec3(u_Model * vec4(aPos, 1.0));
    Normal = vec3(u_NormalMatrix * vec4(aNormal, 0.0)); // 注意法线不进行平移
    ViewDir = normalize(u_ViewPos - FragPos);
    gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);
}

#shader fragment
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 ViewDir;

uniform vec3 u_LightPos;
uniform vec3 u_ObjectColor;
uniform vec3 u_LightColor;
uniform vec3 u_AmbientColor;
uniform vec3 u_SpecularColor;

uniform float u_Constant;
uniform float u_Linear;
uniform float u_Quadratic;

uniform float u_Shininess;

void main()
{
    // 环境光
    vec3 ambient = u_AmbientColor * u_ObjectColor; // 环境光颜色 * 物体颜色

    // 光照衰减
    float distance = length(u_LightPos - FragPos);
    float attenuation = 1.0 / (u_Constant + u_Linear * distance + u_Quadratic * (distance * distance));
    vec3 attenuatedLightColor = u_LightColor * attenuation;

    // 漫反射
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * attenuatedLightColor * u_ObjectColor;

    // 镜面反射
    vec3 halfwayDir = normalize(lightDir + ViewDir);
    float spec = max(pow(dot(norm,halfwayDir),u_Shininess),0.0);
    vec3 specular = spec * attenuatedLightColor * u_SpecularColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
} 