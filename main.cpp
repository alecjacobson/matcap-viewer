#include <igl/read_triangle_mesh.h>
#include <igl/sort_triangles.h>
#include <igl/png/readPNG.h>
#include <igl/opengl/glfw/Viewer.h>
#include <igl/opengl/create_shader_program.h>
#include <igl/opengl/destroy_shader_program.h>
#include <Eigen/Core>

int main(int argc, char *argv[])
{
  using namespace Eigen;
  using namespace std;
  Eigen::MatrixXd V;
  Eigen::MatrixXi F;
  igl::opengl::glfw::Viewer v;

  for(int i = 1;i<argc;i+=2)
  {
  igl::read_triangle_mesh(argv[i], V, F);
  Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic> R,G,B,A;
  igl::png::readPNG(argv[i+1],R,G,B,A);
  // Set background to matcap background (average)

  v.data().set_mesh(V,F);
  v.data().set_texture(R,G,B,A);
  v.data().set_face_based(false);
  v.data().show_lines = false;
  v.data().show_texture = true;
  if(i==1)
  {
    v.launch_init(true,false);
    Eigen::Vector3f average(0,0,0);
    int k = 0, r = R.rows(), c = R.cols();
    for(int i=0;i<r;i++) 
    for(int j=0;j<c;j++)
    if((2.0*i-r)*(2.0*i-r)+(2.0*j-c)*(2.0*j-c)>r*r)
    {
      average += Eigen::Vector3f(R(i,j),G(i,j),B(i,j))/255.0;
      k++;
    }
    v.core().background_color.head(3) = average/double(k);
  }
  v.callback_mouse_up = 
    [&](igl::opengl::glfw::Viewer &, int button, int mod)
  {
    Eigen::VectorXi _;
    igl::sort_triangles(V,Eigen::MatrixXi(F), v.core().view, v.core().proj,F,_);
    v.data().set_mesh(V,F);
    v.data().compute_normals();
    return false;
  };

  v.data().meshgl.init();
  igl::opengl::destroy_shader_program(v.data().meshgl.shader_mesh);

  {
    std::string mesh_vertex_shader_string =
R"(#version 150
uniform mat4 view;
uniform mat4 proj;
uniform mat4 normal_matrix;
in vec3 position;
in vec3 normal;
out vec3 normal_eye;

void main()
{
  normal_eye = normalize(vec3 (normal_matrix * vec4 (normal, 0.0)));
  gl_Position = proj * view * vec4(position, 1.0);
})";

    std::string mesh_fragment_shader_string =
R"(#version 150
in vec3 normal_eye;
out vec4 outColor;
uniform sampler2D tex;
void main()
{
  vec2 uv = normalize(normal_eye).xy * 0.5 + 0.5;
  outColor = texture(tex, uv);
})";

    igl::opengl::create_shader_program(
      mesh_vertex_shader_string,
      mesh_fragment_shader_string,
      {},
      v.data().meshgl.shader_mesh);
  }
    if(i+2 < argc)
{
  std::cout<<"append_mesh"<<std::endl;
  v.append_mesh();
}
  }

  v.callback_mouse_up(v,0,0);
  v.launch_rendering(true);
  v.launch_shut();
}
