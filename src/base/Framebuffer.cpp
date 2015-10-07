#include "Framebuffer.hpp"
#include <iostream>

using namespace std;
using namespace FW;

void Framebuffer::create(Vec2i size_in)
{
	glGenFramebuffers(1, &ID);
	size = size_in;
	GLContext::checkErrors();
}

void Framebuffer::addtexturearray(GLenum index, GLenum type, int levels)
{
	texturearraysize = levels;
	{
		texturearray.index = index;
		texturearray.type = type;
		glGenTextures(1, &texturearray.ID);

		glBindTexture(GL_TEXTURE_2D_ARRAY, texturearray.ID);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

		switch (type)
		{
		case(GL_RGBA32F) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, size.x, size.y, texturearraysize, 0, GL_RGBA, GL_FLOAT, NULL);
			break;
		case(GL_R32F) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, size.x, size.y, texturearraysize, 0, GL_RED, GL_FLOAT, NULL);
			break;
		case(GL_RGBA16F) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16F, size.x, size.y, texturearraysize, 0, GL_RGBA, GL_FLOAT, NULL);
			break;
		case(GL_R16F) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R16F, size.x, size.y, texturearraysize, 0, GL_RED, GL_FLOAT, NULL);
			break;
		case(GL_RGBA16) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16, size.x, size.y, texturearraysize, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
			break;
		case(GL_RGBA8) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, size.x, size.y, texturearraysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			break;
		case(GL_DEPTH_COMPONENT32) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32, size.x, size.y, texturearraysize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			break;
		default:
			cout << "Invalid texture type in framebuffer::addtexturearray."<< endl;
			break;
		}
	}
}

void Framebuffer::addtexturetarget(string name, GLenum index, GLenum type)
{
	texinfo tex;
	tex.index = index;
	tex.type = type;
	glGenTextures(1, &tex.ID);

	glBindTexture(GL_TEXTURE_2D, tex.ID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	switch (type)
	{
	case(GL_RGBA32F) :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, NULL);
		break;
	case(GL_R32F) :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, size.x, size.y, 0, GL_RED, GL_FLOAT, NULL);
		break;
	case(GL_RGBA16F) :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, NULL);
		break;
	case(GL_R16F) :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, size.x, size.y, 0, GL_RED, GL_FLOAT, NULL);
		break;
	case(GL_RGBA16) :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
		break;
	case(GL_RGBA8) :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		break;
	case(GL_DEPTH_COMPONENT32) :
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
		break;
	default:
		cout << "Invalid texture type in framebuffer::addtexturetarget. Texture name: " << name << endl;
		break;
	}

	GLContext::checkErrors();
	textures[name] = tex;
}

void Framebuffer::resize(Vec2i newsize)
{
	size = newsize;
	for (std::map<string, texinfo>::iterator it = textures.begin(); it != textures.end(); ++it)
	{
		texinfo tex = it->second;
		glBindTexture(GL_TEXTURE_2D, tex.ID);
		switch (tex.type)
		{
		case(GL_RGBA32F) :
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, NULL);
			break;
		case(GL_R32F) :
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, size.x, size.y, 0, GL_RED, GL_FLOAT, NULL);
			break;
		case(GL_RGBA16F) :
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.x, size.y, 0, GL_RGBA, GL_FLOAT, NULL);
			break;
		case(GL_R16F) :
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, size.x, size.y, 0, GL_RED, GL_FLOAT, NULL);
			break;
		case(GL_RGBA16) :
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
			break;
		case(GL_RGBA8) :
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			break;
		case(GL_DEPTH_COMPONENT32) :
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, size.x, size.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			break;
		default:
			cout << "Invalid texture type in framebuffer::addtexturetarget. Texture index: " << tex.index << endl;
			break;
		}
	}
	auto tex = texturearray;
	if (tex.ID != -1)
	{
		switch (tex.type)
		{
		case(GL_RGBA32F) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA32F, size.x, size.y, texturearraysize, 0, GL_RGBA, GL_FLOAT, NULL);
			break;
		case(GL_R32F) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R32F, size.x, size.y, texturearraysize, 0, GL_RED, GL_FLOAT, NULL);
			break;
		case(GL_RGBA16F) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16F, size.x, size.y, texturearraysize, 0, GL_RGBA, GL_FLOAT, NULL);
			break;
		case(GL_R16F) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R16F, size.x, size.y, texturearraysize, 0, GL_RED, GL_FLOAT, NULL);
			break;
		case(GL_RGBA16) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA16, size.x, size.y, texturearraysize, 0, GL_RGBA, GL_UNSIGNED_SHORT, NULL);
			break;
		case(GL_RGBA8) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, size.x, size.y, texturearraysize, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
			break;
		case(GL_DEPTH_COMPONENT32) :
			glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_DEPTH_COMPONENT32, size.x, size.y, texturearraysize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
			break;
		default:
			cout << "Invalid texture type in framebuffer::addtexturetarget. Texture index: " << tex.index << endl;
			break;
		}
	}
	GLContext::checkErrors();
}

void Framebuffer::bindtexturearray(int layer)
{
	glViewport(.0f, .0f, size.x, size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, ID);
	auto tex = texturearray;
	glFramebufferTexture(GL_FRAMEBUFFER, tex.index, tex.ID, 0);
	//glFramebufferTexture3D(GL_FRAMEBUFFER, tex.index, GL_TEXTURE_2D_ARRAY, tex.ID, 0, layer);
	if (tex.index != GL_DEPTH_ATTACHMENT)
		glDrawBuffer(tex.index);
	GLContext::checkErrors();
}

void Framebuffer::bind()
{
	glViewport(.0f, .0f, size.x, size.y);
	glBindFramebuffer(GL_FRAMEBUFFER, ID);
	int num = 0;
	for (std::map<string, texinfo>::iterator it = textures.begin(); it != textures.end(); ++it)
	{
		texinfo tex = it->second;
		if (tex.index != GL_DEPTH_ATTACHMENT)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, tex.index, GL_TEXTURE_2D, tex.ID, 0);
			num++;
		}
		else
			glFramebufferTexture2D(GL_FRAMEBUFFER, tex.index, GL_TEXTURE_2D, tex.ID, 0);
	}
	GLContext::checkErrors();

	GLenum *drawbuffers = new GLuint[num];
	int i = 0;
	for (std::map<string, texinfo>::iterator it = textures.begin(); it != textures.end(); ++it)
	{
		texinfo tex = it->second;
		if (tex.index != GL_DEPTH_ATTACHMENT)
		{
			drawbuffers[i] = tex.index;
			i++;
		}
	}

	glDrawBuffers(num, drawbuffers);
	GLContext::checkErrors();
	free(drawbuffers);
}