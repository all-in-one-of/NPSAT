#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include <vector>
#include <math.h>
#include <string>

#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/fe_values.h>
#include <deal.II/fe/mapping_q1.h>
#include <deal.II/lac/constraint_matrix.h>
#include <deal.II/lac/trilinos_vector.h>

/*!
 * \brief linspace generate a linearly spaced vector between the two numbers min and max
 * \param min is the lower end
 * \param max is the upper end
 * \param n is the number of numbers to generate between the min and max
 * \return a vector of n numbers linearly spaced
 */
std::vector<double> linspace(double min, double max, int n){
    std::vector<double> result;
    int iterator = 0;
    for (int i = 0; i <= n-2; i++){
        double temp = min + i*(max-min)/(floor(static_cast<double>(n)) - 1);
        result.insert(result.begin() + iterator, temp);
        iterator += 1;
    }
    result.insert(result.begin() + iterator, max);
    return result;
}

template <int dim>
bool try_mapping(dealii::Point<dim> p, dealii::Point<dim> &p_unit,
                 typename dealii::Triangulation<dim>::active_cell_iterator cell, dealii::MappingQ1<dim> mapping){
    bool mapping_done = false;
    int count_try = 0;
    dealii::Point<dim> p_try = p;
    while (!mapping_done){
        try{
            p_unit = mapping.transform_real_to_unit_cell(cell, p_try);
            mapping_done = true;
        }
        catch(...){
            for (unsigned int idim = 0; idim < dim; ++idim)
                p_try[idim] = p[idim] + 0.0001*(-1.0 + 2.0*(double(rand())/double(RAND_MAX)));
            ++count_try;
            if (count_try > 20){
                break;
                std::cerr << "transformation Failed" << std::endl;
            }

        }
    }
    return mapping_done;
}

/*!
 * \brief is_input_a_scalar check if the string can be converted into a scalar value
 * \param input is the string to test
 * \return true if the input can be a scalars
 */
bool is_input_a_scalar(std::string input){
    // try to convert the input to scalar
    bool outcome;
    try{
        double value = stod(input);
        value++; // something to surpress the warning
        outcome = true;
    }
    catch(...){
        outcome = false;
    }
    return outcome;
}

/*!
 * \brief triangle_area Calculates the area of a triangle defined by the three vertices
 * \param A 1st Point of triangle
 * \param B 2nd Point of triangle
 * \param C 3rd Point of triangle
 * \param project if true this will calculate the area of the triangle when projected in the XY plane
 * \return the area of the triangle
 */
template <int dim>
double triangle_area(dealii::Point<dim> A, dealii::Point<dim> B, dealii::Point<dim> C, bool project){
    double area = 0;
    if (project){
        area = std::abs( 0.5*(A[0]*(B[1] - C[1]) + B[0]*(C[1] - A[1]) + C[0]*(A[1] - B[1])));
    }
    else{
        //http://mathworld.wolfram.com/TriangleArea.html
        double x1 = A[0]; double y1 = A[1]; double z1 = A[2];
        double x2 = B[0]; double y2 = B[1]; double z2 = B[2];
        double x3 = C[0]; double y3 = C[1]; double z3 = C[2];
        area = pow(x1*y2 - x2*y1 - x1*y3 + x3*y1 + x2*y3 - x3*y2, 2) +
               pow(x1*z2 - x2*z1 - x1*z3 + x3*z1 + x2*z3 - x3*z2, 2) +
               pow(y1*z2 - y2*z1 - y1*z3 + y3*z1 + y2*z3 - y3*z2, 2);
        area = 0.5*sqrt(area);
    }
    return area;
}


/*!
 * \brief recharge_weight calculates a weight factor which is used during recharge calculations.
 * To calculate the correct amount of groundwater recharge (e.g. from precipitation), the recharge rate
 * should be multiplied by the projected area on the xy plane of the top face of the element.
 * \param cell is the element where the recharge is applied
 * \param face is the face id
 * \return the weight
 */
template<int dim>
double recharge_weight(typename dealii::DoFHandler<dim>::active_cell_iterator cell, unsigned int face){
    double weight = 1;
    if (dim == 2){
        dealii::Point<dim> v1 = cell->face(face)->vertex(0);
        dealii::Point<dim> v2 = cell->face(face)->vertex(1);
        double actual_length = v1.distance(v2);
        double projected_length = std::abs(v2[0] - v1[0]);
        weight = projected_length/actual_length;
    }else if (dim == 3){
        dealii::Point<dim> v1 = cell->face(face)->vertex(0);
        dealii::Point<dim> v2 = cell->face(face)->vertex(1);
        dealii::Point<dim> v3 = cell->face(face)->vertex(2);
        dealii::Point<dim> v4 = cell->face(face)->vertex(3);
        double A_real = triangle_area(v1,v2,v4,false) + triangle_area(v1,v4,v3,false);
        double A_proj = triangle_area(v1,v2,v4,true)  + triangle_area(v1,v4,v3,true);
        weight = A_proj/A_real;
    }
    return weight;
}

/*!
 * \brief This function returns a list of indices that are connected to the #ii node in a cell
 * \param #ii is the index of the node we seek its connected nodes
 * \return a vector of connected indices. Note that this is simply returns the ids of the nodes
 * as they are defined by the geometry info class. As we are interested only in the connections
 * along the vertical direction we return only those. However inside the function there is boolean flag
 * that can be hardcoded to return all connections.
 */
template <int dim>
std::vector<int> get_connected_indices(int ii){
    bool return_all = false;
    std::vector<int> out;
    if (dim == 2){
        if (return_all){
            if (ii == 0){
                out.push_back(1);
                out.push_back(2);
            }else if (ii == 1){
                out.push_back(0);
                out.push_back(3);
            }else if (ii == 2){
                out.push_back(0);
                out.push_back(3);
            }else if (ii == 3){
                out.push_back(1);
                out.push_back(2);
            }
        }
        else{
            if (ii == 0){
                out.push_back(2);
            }else if (ii == 1){
                out.push_back(3);
            }else if (ii == 2){
                out.push_back(0);
            }else if (ii == 3){
                out.push_back(1);
            }

        }
    }else if (dim == 3){
        if (return_all){
            if (ii == 0){
                out.push_back(1);
                out.push_back(2);
                out.push_back(4);
            }else if (ii == 1){
                out.push_back(0);
                out.push_back(3);
                out.push_back(5);
            }else if (ii == 2){
                out.push_back(0);
                out.push_back(3);
                out.push_back(6);
            }else if (ii == 3){
                out.push_back(1);
                out.push_back(2);
                out.push_back(7);
            }else if (ii == 4){
                out.push_back(0);
                out.push_back(5);
                out.push_back(6);
            }else if (ii == 5){
                out.push_back(1);
                out.push_back(4);
                out.push_back(7);
            }else if (ii == 6){
                out.push_back(2);
                out.push_back(4);
                out.push_back(7);
            }else if (ii == 7){
                out.push_back(3);
                out.push_back(5);
                out.push_back(6);
            }
        }
        else{
            if (ii == 0){
                out.push_back(4);
            }else if (ii == 1){
                out.push_back(5);
            }else if (ii == 2){
                out.push_back(6);
            }else if (ii == 3){
                out.push_back(7);
            }else if (ii == 4){
                out.push_back(0);
            }else if (ii == 5){
                out.push_back(1);
            }else if (ii == 6){
                out.push_back(2);
            }else if (ii == 7){
                out.push_back(3);
            }
        }
    }
    return out;
}

template <int dim>
void Print_Mesh_DofHandler(std::string filename,
                           unsigned int my_rank,
                           DoFHandler<dim>& mesh_dof_handler,
                           FESystem<dim>& mesh_fe){

    std::map<int,std::pair<int,Point<dim>>> Points; // <dof> - <counter - Point>
    std::map<int,std::vector<int> > Mesh;

    typename std::map<int,std::pair<int,Point<dim>>>::iterator itp;

    int p_cnt = 0; // Point counter
    int m_cnt = 0; // Mesh counter

    const MappingQ1<dim> mapping;
    const std::vector<Point<dim> > mesh_support_points
                                  = mesh_fe.base_element(0).get_unit_support_points();
    FEValues<dim> fe_mesh_points (mapping,
                                  mesh_fe,
                                  mesh_support_points,
                                  update_quadrature_points);

    std::vector<unsigned int> cell_dof_indices (mesh_fe.dofs_per_cell);
    typename DoFHandler<dim>::active_cell_iterator
    cell = mesh_dof_handler.begin_active(),
    endc = mesh_dof_handler.end();
    for (; cell != endc; ++cell){
        if (cell->is_locally_owned()){
            fe_mesh_points.reinit(cell);
            cell->get_dof_indices (cell_dof_indices);
            for (unsigned int idof = 0; idof < mesh_fe.base_element(0).dofs_per_cell; ++idof){
                Point <dim> current_node;
                std::vector<int> current_dofs(dim);
                for (unsigned int dir = 0; dir < dim; ++dir){
                    unsigned int support_point_index = mesh_fe.component_to_system_index(dir, idof );
                    current_dofs[dir] = static_cast<int>(cell_dof_indices[support_point_index]);
                    current_node[dir] = fe_mesh_points.quadrature_point(idof)[dir];
                }
                itp = Points.find(current_dofs[dim-1]);
                if (itp == Points.end()){
                    Points.insert(std::pair<int, std::pair<int,Point<dim>>>(current_dofs[dim-1], std::pair<int,Point<dim>>(p_cnt,current_node)));
                    p_cnt++;
                }
            }
        }
    }

}

#endif // HELPER_FUNCTIONS_H
