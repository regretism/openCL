#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <CL/cl.h>
#include <CL/cl_gl.h>
#include <GL/glut.h>
#include <math.h>
#include <stdio.h>

#define WIDTH 800
#define HEIGHT 600

const char* kernelSource =
"__kernel void render(__write_only image2d_t output)\n"
"{\n"
"    int gid_x = get_global_id(0);\n"
"    int gid_y = get_global_id(1);\n"
"    float x = (gid_x - get_image_width(output) / 2.0) / (get_image_width(output) / 2.0);\n"
"    float y = (gid_y - get_image_height(output) / 2.0) / (get_image_height(output) / 2.0);\n"
"    float z = sqrt(1.0 - x*x - y*y);\n"
"    write_imagef(output, (int2)(gid_x, gid_y), (float4)(z, z, z, 1.0f));\n"
"}\n";

cl_platform_id platform;
cl_device_id device;
cl_context context;
cl_command_queue queue;
cl_program program;
cl_kernel kernel;

GLuint textureID;
cl_mem clTexture;

void initializeCL() {
    clGetPlatformIDs(1, &platform, NULL);
    clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    context = clCreateContext(NULL, 1, &device, NULL, NULL, NULL);
    queue = clCreateCommandQueue(context, device, 0, NULL);

    program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, NULL);
    clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    kernel = clCreateKernel(program, "render", NULL);

    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    cl_int err;
    clTexture = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, textureID, &err);
    if (err != CL_SUCCESS) {
        printf("Error creating OpenCL texture from OpenGL texture: %d\n", err);
        exit(1);
    }
}

void renderCL() {
    size_t globalWorkSize[] = { WIDTH, HEIGHT };
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &clTexture);
    clEnqueueAcquireGLObjects(queue, 1, &clTexture, 0, NULL, NULL);
    clEnqueueNDRangeKernel(queue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
    clEnqueueReleaseGLObjects(queue, 1, &clTexture, 0, NULL, NULL);
    clFinish(queue);
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(0, 0);
    glTexCoord2f(1, 0); glVertex2f(WIDTH, 0);
    glTexCoord2f(1, 1); glVertex2f(WIDTH, HEIGHT);
    glTexCoord2f(0, 1); glVertex2f(0, HEIGHT);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    glFlush();
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("OpenCL Sphere Visualization");

    initializeCL();
    renderCL();

    glMatrixMode(GL_PROJECTION);
    gluOrtho2D(0, WIDTH, 0, HEIGHT);
    glutDisplayFunc(display);
    glutMainLoop();

    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    glDeleteTextures(1, &textureID);
    clReleaseMemObject(clTexture);

    return 0;
}
