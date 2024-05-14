#include "tgaimage.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>//pour random num, abs
#include <ctime>
#include "model.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;

struct Vertex {
    float x, y, z;
    TGAColor color;
    
    Vertex(float x, float y, float z, TGAColor color) : x(x), y(y), z(z), color(color) {}
    
    Vertex() : x(0), y(0), z(0), color(white) {}//valeur initial
};

struct Face {
    std::vector<int> vertices;
};

void drawline(int x0, int x1, int y0, int y1, TGAImage &tga){
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        //if k > 1, swap x and y
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        // make sure to draw from left to right
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = y1 - y0;
    float derror = std::abs(dy*1.0f/dx);
    float error = 0.0f; 
    int y = y0;
    //draw line
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            // if steep，change back x and y
            tga.set(y, x, red);
        } else {
            tga.set(x, y, red);
        }
        error += derror;
        if (error >= 0.5f) {
            y += (y1 > y0 ? 1 : -1);
            error -= 1.0f;
        }
    }
}

float AireTriangle(Vertex v1, Vertex v2, Vertex v3){
    return 0.5*(abs((v2.x-v1.x)*(v3.y-v1.y)-(v3.x-v1.x)*(v2.y-v1.y)));
}

float AireTriangle(int x1, int y1, Vertex v2, Vertex v3){
    return 0.5*(abs((v2.x-x1)*(v3.y-y1)-(v3.x-x1)*(v2.y-y1)));
}

//alpha beta gamma of triangle
std::vector<float> getLambda(int px, int py, Vertex v1, Vertex v2, Vertex v3){
    std::vector<float> lambda(3);
    lambda[0]=AireTriangle(px,py,v2,v3)/AireTriangle(v1,v2,v3);
    lambda[1]=AireTriangle(px,py,v1,v3)/AireTriangle(v1,v2,v3);
    lambda[2]=AireTriangle(px,py,v1,v2)/AireTriangle(v1,v2,v3);
    return lambda;
}

TGAColor mixColor(const TGAColor& color1, float lambda1 ,const TGAColor& color2, float lambda2 ,const TGAColor& color3, float lambda3) {
    return TGAColor(static_cast<unsigned char>(color1.r * lambda1 + color2.r * lambda2 + color3.r * lambda3),
                    static_cast<unsigned char>(color1.g * lambda1 + color2.g * lambda2 + color3.g * lambda3),
                    static_cast<unsigned char>(color1.b * lambda1 + color2.b * lambda2 + color3.b * lambda3),
                    255);
}


void drawTriangle(Vertex v1, Vertex v2, Vertex v3, TGAImage &tga) {
    //drawline(v1.x, v1.y, v2.x, v2.y, tga);
    //drawline(v2.x, v2.y, v3.x, v3.y, tga);
    //drawline(v3.x, v3.y, v1.x, v1.y, tga);
    
    float max_x, max_y, min_x, min_y;
    max_x=std::max({v1.x, v2.x, v3.x});
    min_x=std::min({v1.x, v2.x, v3.x});
    max_y=std::max({v1.y, v2.y, v3.y});
    min_y=std::min({v1.y, v2.y, v3.y});
    
    std::cout <<"max x:"<<max_x<<"\n";
    std::cout <<"max y:"<<max_y<<"\n";
    std::cout <<"min x:"<<min_x<<"\n";
    std::cout <<"min y:"<<min_y<<"\n";
    
    std::vector<float> lambda(3);
    for(int x=static_cast<int>(min_x); x<static_cast<int>(max_x); x++){
        for(int y=static_cast<int>(min_y); y<static_cast<int>(max_y); y++){
            lambda=getLambda(x,y,v1,v2,v3);
            if((lambda[0]+lambda[1]+lambda[2])<=1){//p is inside triangle
                TGAColor color = mixColor(v1.color,lambda[0],v2.color, lambda[1],v3.color, lambda[2]);
                tga.set(x,y,color);
            }
        }
    }
}

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

int main(int argc, char** argv) {
    srand(time(NULL));
	TGAImage image(width, height, TGAImage::RGB);
    
	if (argc < 2){
		 std::cout << "Entrer a .OBJ file! or modify the makefile \n"<< std::flush;
	}
	std::ifstream objFile(argv[1]);
    std::string line;
    std::vector<Vertex> vertices;
    std::vector<Face> faces;

    if (!objFile.is_open()) {
        std::cerr << "Failed to open file\n";
        return 1;
    }
    //get all vertice and face
	while (std::getline(objFile, line)) {
        std::istringstream lineStream(line);
        std::string type;
        lineStream >> type;
        if (type == "v") {
            Vertex vertex;
            lineStream >> vertex.x >> vertex.y >> vertex.z;
            vertex.color = TGAColor(rand() % 255, rand() % 255, rand() % 255, 255);//random color for each vertex
            vertices.push_back(vertex);
        } else if (type == "f") {
            Face face;
            int vertexIndex;
            while (lineStream >> vertexIndex) {
                // .obj indice start with 1
                face.vertices.push_back(vertexIndex - 1);

            }
            faces.push_back(face);
        }
    }

    std::cout << "finished reading obj file \n"<< std::flush;
    
    
    float minX = std::numeric_limits<float>::max(),
        maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max(),
        maxY = std::numeric_limits<float>::lowest();

    // find min max of all x y
    for (const auto& vertex : vertices) {
        minX = std::min(minX, vertex.x);
        maxX = std::max(maxX, vertex.x);
        minY = std::min(minY, vertex.y);
        maxY = std::max(maxY, vertex.y);
    }

    std::vector<Vertex> newVertices(vertices.size());
    /*
    //adjust position of vertex in tgaImage
    for (const auto& vertex : vertices) {
        float x = (vertex.x - minX) / (maxX - minX) * (image.get_width() - 1);
        float y = (vertex.y - minY) / (maxY - minY) * (image.get_height() - 1);
        image.set(static_cast<int>(x), static_cast<int>(y), vertex.color);
    }
    
    //draw vertices and segment for each face
    for (const Face &face : faces) {
        for (int i = 0; i < face.vertices.size(); i += 3) {
            // 3 indice of triangle
            int idx1 = face.vertices[i];
            int idx2 = face.vertices[i + 1];
            int idx3 = face.vertices[i + 2];

            //
            Vertex old_v1 = vertices[idx1];
            Vertex old_v2 = vertices[idx2];
            Vertex old_v3 = vertices[idx3];
            
            
            Vertex v1 = {
                (old_v1.x - minX) / (maxX - minX) * (image.get_width() - 1),
                (old_v1.y - minY) / (maxY - minY) * (image.get_height() - 1),
                0,
                old_v1.color
            };
            Vertex v2 = {
                (old_v2.x - minX) / (maxX - minX) * (image.get_width() - 1),
                (old_v2.y - minY) / (maxY - minY) * (image.get_height() - 1),
                0,
                old_v2.color
            };
            Vertex v3 = {
                (old_v3.x - minX) / (maxX - minX) * (image.get_width() - 1),
                (old_v3.y - minY) / (maxY - minY) * (image.get_height() - 1),
                0,
                old_v3.color
            };

            // draw triangle
            drawTriangle(v1, v2, v3, image);
        }
    }*/
    
    //test drawtriangle
    Vertex v1(50.,50.,50.,white); Vertex v2(150.,100.,50.,red); Vertex v3(100.,50.,50.,white);
    drawTriangle(v1,v2,v3, image);
    std::cout << "finished draw triangle \n"<< std::flush;
    
    std::cout << "lambda(55,53) : ";
    std::vector<float> lambda = getLambda(55,53,v1,v2,v3);
    for (const auto& val : lambda) {
        std::cout << val << " ";
    }
    //drawline(13,29,34,23,image);
	//image.set(52, 41, red);
	image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
	image.write_tga_file("output.tga");
	return 0;
}

