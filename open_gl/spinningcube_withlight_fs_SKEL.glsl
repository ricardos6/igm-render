#version 130

struct Material {
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

struct Light {
  vec3 position;
  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

out vec4 frag_col;

in vec3 frag_3Dpos;
in vec3 normal;
in vec2 tex_coord;

uniform Material material;
uniform Light light;
uniform vec3 view_pos;

void main() {
  // Ambient
  vec3 ambient = light.ambient * vec3(texture(material.diffuse, tex_coord));

  vec3 light_dir = normalize(light.position - frag_3Dpos);

  // Diffuse
  float diff = max(dot(normal, light_dir), 0.0);
  vec3 diffuse = diff * light.diffuse * vec3(texture(material.diffuse, tex_coord));
  
  // Specular
  vec3 view_dir = normalize(view_pos - frag_3Dpos);
  vec3 reflect_dir = reflect(-light_dir, normal);
  float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
  vec3 specular = light.specular * spec * vec3(texture(material.specular, tex_coord));

  vec3 result = ambient + diffuse + specular;
  frag_col = vec4(result, 1.0);
}
