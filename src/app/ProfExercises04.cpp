#include <iostream>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glm/gtx/transform.hpp"
using namespace std;

int main(int argc, char **argv) {
    cout << "GLM EXERCISES COMMENCE!!!!" << endl;

    glm::vec3 A = glm::vec3(2,6,-3);
    glm::vec3 B = glm::vec3(1,9,2);
    // From A to B
    glm::vec3 AtoB = B - A;
    cout << "A: " << glm::to_string(A) << endl;
    cout << "B: " << glm::to_string(B) << endl;
    cout << "AtoB: " << glm::to_string(AtoB) << endl;

    glm::vec3 color(1,1,0);
    glm::vec4 out_color = glm::vec4(color, 1.0);

    float lenA = glm::length(A);
    glm::vec3 normA = glm::normalize(A);
    cout << "Length of A: " << lenA << endl;
    cout << "Normalized A: " 
            << glm::to_string(normA) << endl;

    cout << "Dot(A,B): " << glm::dot(A,B) << endl;
    glm::vec3 crossAB = glm::cross(A,B);
    cout << "A x B: " << glm::to_string(crossAB) << endl;


    return 0;
}