
#include "gui/Window.hpp"
#include <string>
#include <map>
#include <vector>

namespace FW
{
	struct texinfo
	{
		GLuint ID = -1;
		GLenum index;
		GLenum type;
	};
	class Framebuffer
	{
	public:
		GLuint ID;
		std::map<std::string, texinfo> textures;
		texinfo texturearray;
		int texturearraysize;
		Vec2i size;

		void	create(Vec2i size);
		void	addtexturetarget(std::string name, GLenum index, GLenum type);
		void	addtexturearray(GLenum index, GLenum type, int levels);
		void	bind();
		void	bindtexturearray(int layer);
		void	resize(Vec2i newsize);
	};
}