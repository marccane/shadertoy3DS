#include <3ds.h>
#include <citro3d.h>
#include <string.h>
#include "vshader_shbin.h"

#define CLEAR_COLOR 0x68B0D8FF

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

typedef struct { float position[3]; float color[4]; } vertex;

//static const float DEF_COLOR[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
//whatever
#define DEF_COLOR { 1.0f, 0.0f, 0.0f, 1.0f }
#define RED_COLOR { 1.0f, 0.0f, 0.0f, 1.0f }
#define GREEN_COLOR { 0.0f, 1.0f, 0.0f, 1.0f }
#define BLUE_COLOR { 0.0f, 0.0f, 1.0f, 1.0f }

// static const vertex vertex_list[] =
// {
// 	{ { 200.0f, 200.0f, 0.5f }, DEF_COLOR },
// 	{ { 100.0f, 40.0f, 0.5f }, DEF_COLOR },
// 	{ { 300.0f, 40.0f, 0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
// };

#define vertex_list_count (sizeof(vertex_list)/sizeof(vertex_list[0]))

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection;
static C3D_Mtx projection;

static void* vbo_data;

static void sceneInit(void)
{
	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_shbin, vshader_shbin_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);

	// Get the location of the uniforms
	uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");

	// Configure attributes for use with the vertex shader
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 4); // v1=color
	//AttrInfo_AddFixed(attrInfo, 1); // v1=color

	// Set the fixed attribute (color) to solid white
	//C3D_FixedAttribSet(1, 1.0, 1.0, 1.0, 1.0);

	// Compute the projection matrix
	Mtx_OrthoTilt(&projection, 0.0, 400.0, 0.0, 240.0, 0.0, 1.0, true);

	// Dinamically create the triangles
	//const int TOTAL_TRIANGLES = 266, UL_TRIS_PER_ROW = 133, BL_TRIS_PER_ROW = UL_TRIS_PER_ROW;
	#define TOTAL_TRIANGLES 266
	#define UL_TRIS_PER_ROW 133
	//static so it gets allocated in the .data section not on stack
	static vertex din_vertex_list[TOTAL_TRIANGLES*3]; //TODO: Make sure this is the right number of vertices
	//podem fer primer tots els triangles "UL facing" i després els "DR facing"

	int BLs = 1;
	#define DEF_DEPTH 0.5f
	#define OFF 2.1f
	//UL-facing triangles should be: (First row, (x,y))
	//[[(0,0),(1,0),(0,1)],[(3,0),(4,0),(3,1)],...]
	//Second row:
	//[[(0,2),(1,2),(0,3)],[(3,2),(4,2),(3,3)],...]
	if(BLs)
		for(int i=0;i<UL_TRIS_PER_ROW;++i){
			//JUST THE FIRST ROW
			float bx = i*3.0f; //baseX
			din_vertex_list[i*3] 	= (vertex) { { bx, 		0.0f, 	0.5f }, RED_COLOR };
			din_vertex_list[i*3+1] 	= (vertex) { { bx+OFF, 	0.0f, 	0.5f }, GREEN_COLOR };
			din_vertex_list[i*3+2] 	= (vertex) { { bx, 		OFF, 0.5f }, BLUE_COLOR };
		}
	//DR-facing triangles should be:
	//[[(2,0),(1,1),(2,1)],[(5,0),(4,1),(5,1)],...]
	//Second row:
	//[[(2,2),(1,3),(2,3)],[(5,2),(4,3),(5,3)],...]
	else
		for(int i=0;i<UL_TRIS_PER_ROW;++i){
			//JUST THE FIRST ROW
			int idx_base = 0; //UL_TRIS_PER_ROW
			float bx = i*3.0f; //baseX
			din_vertex_list[idx_base+i*3] 	= (vertex) { { bx+2.5f,	0.0f, 	0.5f }, BLUE_COLOR }; //Botom vertex
			din_vertex_list[idx_base+i*3+2] 	= (vertex) { { bx+1.0f, 2.1f, 	0.5f }, GREEN_COLOR }; //Top left vertex
			din_vertex_list[idx_base+i*3+1] 	= (vertex) { { bx+2.5f,	2.1f, 	0.5f }, RED_COLOR }; //Top right vertex
		}

	// din_vertex_list[0] 	= (vertex) { { 2.5f,	0.0f, 	0.5f }, RED_COLOR }; //Botom vertex
	// din_vertex_list[2] 	= (vertex) { { 1.0f,	2.1f, 	0.5f }, GREEN_COLOR }; //Top left vertex
	// din_vertex_list[1] 	= (vertex) { { 2.5f,	2.1f, 	0.5f }, BLUE_COLOR }; //Top right vertex

	// din_vertex_list[0] 	= (vertex) { { 3.1f,	0.0f, 	0.5f }, RED_COLOR };
	// din_vertex_list[1] 	= (vertex) { { 1.0f, 2.1f, 	0.5f }, GREEN_COLOR };
	// din_vertex_list[2] 	= (vertex) { { 3.1f,	2.1f, 	0.5f }, BLUE_COLOR };

	//din_vertex_list[0] 	= (vertex) { { 0.0f,	0.0f, 	0.5f }, RED_COLOR };
	// din_vertex_list[0] 	= (vertex) { { 5.1f,	5.1f, 	0.5f }, RED_COLOR };
	// din_vertex_list[1] 	= (vertex) { { OFF, 	0.0f, 	0.5f }, GREEN_COLOR };
	// din_vertex_list[2] 	= (vertex) { { 0.0f,	OFF, 	0.5f }, BLUE_COLOR };

	//Working BL
	// din_vertex_list[0] 	= (vertex) { { 0.0f,	0.0f, 	0.5f }, RED_COLOR };
	// din_vertex_list[1] 	= (vertex) { { OFF, 	0.0f, 	0.5f }, GREEN_COLOR };
	// din_vertex_list[2] 	= (vertex) { { 0.0f,	OFF, 	0.5f }, BLUE_COLOR };

	// din_vertex_list[0] 	= (vertex) { { OFF2,	0.0f, 	0.5f }, RED_COLOR };
	// din_vertex_list[1] 	= (vertex) { { bx+OFF, 	OFF, 	0.5f }, GREEN_COLOR };
	// din_vertex_list[2] 	= (vertex) { { bx+OFF2,	OFF, 	0.5f }, BLUE_COLOR };

	// Create the VBO (vertex buffer object)
	// vbo_data = linearAlloc(sizeof(vertex_list));
	// memcpy(vbo_data, vertex_list, sizeof(vertex_list));
	vbo_data = linearAlloc(sizeof(din_vertex_list));
	memcpy(vbo_data, din_vertex_list, sizeof(din_vertex_list));

	// Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, vbo_data, sizeof(vertex), 2, 0x10); //buffInfo, data, sizeof(a whole vertex attr), num of attributes (non-fixed only?), strange "mask" that depends on the previous parameter to specify in which input register every sub element goes to

	// Configure the first fragment shading substage to just pass through the vertex color
	// See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
	C3D_TexEnv* env = C3D_GetTexEnv(0);
	C3D_TexEnvInit(env);
	C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, 0, 0);
	C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);
}

static void sceneRender(void)
{
	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);

	// Draw the VBO
	C3D_DrawArrays(GPU_TRIANGLES, 0, UL_TRIS_PER_ROW*3*2);
}

static void sceneExit(void)
{
	// Free the VBO
	linearFree(vbo_data);

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
}

int main()
{
	// Initialize graphics
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	// Initialize the render target
	C3D_RenderTarget* target = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetOutput(target, GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);

	// Initialize the scene
	sceneInit();

	// Main loop
	while (aptMainLoop())
	{
		hidScanInput();

		// Respond to user input
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			break; // break in order to return to hbmenu

		// Render the scene
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C3D_RenderTargetClear(target, C3D_CLEAR_ALL, CLEAR_COLOR, 0);
			C3D_FrameDrawOn(target);
			sceneRender();
		C3D_FrameEnd(0);
	}

	// Deinitialize the scene
	sceneExit();

	// Deinitialize graphics
	C3D_Fini();
	gfxExit();
	return 0;
}
