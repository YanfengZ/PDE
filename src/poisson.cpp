#include "../include/poisson.h"

using namespace std;


Poisson::Poisson(double m_error, VectorXi dims, MatrixXd dom, Function fn){
    max_error = m_error;
    dimensions = dims;
    domain = dom;
    h = MatrixXd::Zero(dims.rows(), 1);
    for(int i = 0; i < domain.cols(); i++){
        h(i) = (domain(1,i) - domain(0,i))/(dims(i) + 1);
        }
    func = fn;

    }

void Poisson::set_matrix(){
    int all_dims = dimensions(0)*get_under_dim(1);
    std::vector<T> tripletList;
    tripletList.reserve((2*dimensions.rows()+1)*all_dims);
    double h_factor = 0;
    for(int i = 0; i < h.rows(); i++){
        h_factor += pow(h(i),-2);
        }

    for(int n = 0; n < all_dims; n++){
        tripletList.push_back(T(n,n,2*h_factor));
        for(int dim = 0; dim < dimensions.rows(); dim++){
            int shift_no = get_shift_number(dim);
            if(n-shift_no >= 0){
                tripletList.push_back(T(n,n-shift_no,-pow(h(dim),-2)));
                }
            if(n+shift_no <= all_dims-1){
                tripletList.push_back(T(n,n+shift_no,-pow(h(dim),-2)));
                }
            }
        }

    SparseMatrix<double> mat(all_dims, all_dims);
    mat.setFromTriplets(tripletList.begin(), tripletList.end());
    A = mat;
}

int Poisson::get_shift_number(int max_dim){
    int shift_no = 1;
    for(int i = 0; i < max_dim; i++){
        shift_no *= dimensions(i);
        }
    return shift_no;
}

int Poisson::get_under_dim(int dim_number){

    int underdim = 1;
    for(int i = 0; i < dim_number - 1; i++){
        underdim *= dimensions(i);
        }
    for(int i = dim_number; i < dimensions.rows(); i++){
        underdim *= dimensions(i);
        }
    return underdim;
}

uint Poisson::to_index(VectorXi coords){
    uint index = coords(0);
    int under_dim = 1;
    for(int i = 1; i < coords.rows(); i++){
        under_dim = get_under_dim(i + 1);
        index += coords(i)*under_dim;
        }
    return index;
}

SparseMatrix<double> Poisson::get_diff_matrix() const{
    return A;
}

Vector Poisson::derivative(Vector u) const{
    Vector v = A*u;
    return v*pow(h(0), -2);
}

Vector Poisson::solve(){
    Vector x = MatrixXd::Zero(dimensions(0), 1);
    for(int i = 0; i < dimensions(0); i++){
        x(i) = domain(0) + (1+i)*h(0);
        }
    Vector f = func(x);

    Vector u = MatrixXd::Zero(dimensions(0), 1);
    // fill A and b
    ConjugateGradient<SparseMatrix<double>, Lower|Upper> cg;
    cg.compute(A);
    u = cg.solve(f);
    cout << "#iterations:     " << cg.iterations() << endl;
    cout << "estimated error: " << cg.error()      << endl;
    f = A*u;
    u = cg.solve(f);
    return pow(h(0),2)*u;
}

void Poisson::show() const{
    cout << "A: " << endl;
    cout << A << endl;
    cout << "dimension: " << dimensions(0) << endl;
    cout << "domain: " << domain << endl;
    cout << "h: " << h << endl;
}