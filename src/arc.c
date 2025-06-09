#include "../include/arc.h"


//void Draw_Arc(int x, int y, int r_inner, int r_outer, double start_angle, double end_angle, Color clr) {
//	float fx = (float) x;
//	float fy = (float) y;
//
//	// Generate Vertices
//	int vert_len = 2 * (ARC_RESOLUTION+1);
//	//SDL_Vertex vert[vert_len];
//	Vector2 vert[vert_len];
//	for (int i=0; i<vert_len; i+=2) {
//		double theta = (double)(i)/2.0f * ((end_angle - start_angle)/ARC_RESOLUTION);
//		//vert[i] = (SDL_Vertex){
//		//	.position = {
//		//		fx + SDL_cos(start_angle + theta) * r_inner,
//		//		fy + SDL_sin(start_angle + theta) * r_inner
//		//	},
//		//	.color = clr,
//		//	//.color = (SDL_Color){ 0xFF, 0x00, 0x00, 0xFF },
//		//	//.color = (SDL_Color){ 0x00, 0x00, (Uint8)(255.0f * ((double)i/vert_len)), 0xFF },
//		//	.tex_coord = { 0.0f, 0.0f }
//		//};
//		//vert[i+1] = (SDL_Vertex){
//		//	.position = { 
//		//		fx + SDL_cos(start_angle + theta) * r_outer,
//		//		fy + SDL_sin(start_angle + theta) * r_outer
//		//	},
//		//	.color = clr,
//		//	//.color = (SDL_Color){ 0x00, 0x00, 0xFF, 0xFF },
//		//	//.color = (SDL_Color){ 0x00, 0x00, (Uint8)(255.0f * ((double)i/vert_len)), 0xFF },
//		//	.tex_coord = { 0.0f, 0.0f }
//		//};
//
//		vert[i] = (Vector2){
//			fx + cos(start_angle + theta) * (double)(r_inner),
//			fy + sin(start_angle + theta) * (double)(r_inner)
//		};
//		vert[i+1] = (Vector2){
//			fx + cos(start_angle + theta) * (double)(r_outer),
//			fy + sin(start_angle + theta) * (double)(r_outer)
//		};
//	}
//
//	// Generate Indices
//	//int ind_len = 2 * ARC_RESOLUTION * 3;
//	//int ind[ind_len];
//	//for (int i = 0; i<(2*ARC_RESOLUTION); i++) {
//	//	ind[ i * 3 + 0 ] = i + 0;
//	//	ind[ i * 3 + 1 ] = i + 1;
//	//	ind[ i * 3 + 2 ] = i + 2;
//	//}
//
//	//SDL_RenderGeometry(rend, NULL, vert, vert_len, ind, ind_len);
//	
//	for (int i=0; i<vert_len; i++) printf("---> [%i] %4.4f / %4.4f\n", i, vert[i].x, vert[i].y);
//	for (int i=0; i<vert_len; i++) DrawPoly(vert[i], 4, 5.0f, 0.0f, BLUE);
//	DrawTriangleStrip(vert, vert_len, clr);
//}

