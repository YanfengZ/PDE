# PDE
Implementation of finite element method for solving PDEs in 2-D and 3-D utilizing simplex mesh and pyramid basis functions

1. Usage

First, import Solver.h. Then, in order to use PDE Solver, you also need to define the kernel functions of the weak form of the PDE, i.e.the bilinear kernel (==MatrixXd) for the weak differential operator and the inhomogeneous function f on the right side.
Furthermore, you need to define the three boundary functions in C++ way too; ConditionFn cond_fn,
ConditionFn is_inside and ValueFn val. NormalFn normal is currently not needed but available for eventual future work
with general Robin boundary conditions involving surface integrals.

Secondly, one needs to fill the mesh with Simplices. If the domain is a simple cube, one can utilize MeshFiller
through Solver to fill it with 2 / 6 simplices, depending on the dimensions of the domain. Even in more complicated domains, it's enough to have Simplices with Vertices on the boundary and then the adjustments towards it will be made once refining creates new mid point vertices as long as the surface is convex and smooth (see tests for the 2-D example:
  SECTION("Solving PDE in a unit circle domain should succeed") ).

After all this is done, one can start the process by refining the mesh and giving the amount of available CPU cores
to the Solver's solve method. The result is a Vector of the  for a linear combination of the base functions of degree 1.
One can save the solution coordinates as a matrix or as a Mathematica array for visualization purposes. demo.cpp has a small 2-D Poisson problem with the analytical periodic solution as an example how to use the Solver.

2. Technical Details (see the class diagram pdf)

PDE Solver depends on Eigen library and the parallelization requires C++ 11. Testing the Solver depends on Catch2 unit testing framework. No other dependencies are needed. Since most of the classes are template classes, even the small non-template classes are defined in the header files instead of cpp-files. Importing Solver.h takes care of all the needed templates. Structurally the PDE Solver has two main levels. One for the Mesh with its Elements and another one for the PDE and the Solver iterating over the Mesh Elements.

##Mesh Level
Vertex template class stores coordinates (A Point object or a VectorXd, depending on what you need), index and the amount of elements sharing the vertex. N+1 vertices in a N-Dim space define an element called Simplex. Every Vertex has its own SimplexFunction of degree 1, which has the value 1 at the vertex and 0 at every other vertex. For example, in 2-D, a Vertex can be shared only by 6 Elements so that its support can maximally cover this area. Every SimplexFunction is continuous but generally not differentiable at the edges and corresponds to a Vertex which has a unique index. The IndexMaps object takes care of converting local indices of the vertices/functions vector to global ones and vice versa. Since a Vertex can be shared by many Elements, the Element template class only has access to a vector of Vertex pointers. This is why the implementation differs from "the rule of three", and Vertex destructor is only called once it is shared by no Elements. Elements are stored in MeshNode as data and form a linked list (of MeshNodes), a Mesh which can be iterated over.

New Elements are created in ElementDivider. Simplex is divided first symmetrically in 2^Dim new Vertices. In the process, 1+...+Dim new midpoints are created and they need to be added to a temporary commons map of pointers, where every new midpoint Vertex is a value of the key {I, J} where I, J are the unique indices of the old Vertices. Whenever we find a midpoint Vertex on the boundary, we'll use Divider's find_surface_point method which smoothens the simplex accordingly, either outwards or inwards. This works as long as the boundary does not fluctuate heavily and is smooth.

ElementNodes form a (forward) linked list where we can easily iterate over the Nodes as well as push and pop ElementNodes at arbitrary locations. Refine method uses ElementDivider's to divide to create push new ElementNodes to the Mesh and pop the old ones out. It also stores edge_sharings where every edge is defined as a key value pair of the sort {I,J} -> Amount of Elements sharing the edge. When ever the value is 1 we know that the edge is on the boundary. This is utilized in the indexing of the Elements. All the inner Vertices get an index between 0 and N whereas boundary Vertices (note: a Vertex at the boundary is not necessarily 'an outer Vertex' since it might not have an outer edge attached to it!) get an index larger that N. This information is then used by the ElementDivider to determine if the new Vertex should be moved towards the boundary or not.

##Solver Level
PDE is for storing the weak form of the PDE and for calculating the stiffness matrix elementwise as well as the f vector. Integration in the first case is simply multiplication when the Bilinear kernel A does not depend on the coordinates and we use first degree simplex functions. The second result, the inner product of <f, SimplexFn fn_I> (== integral of f(r)xfn_I(r) over the simplex domain), requires a Monte Carlo integration to be accurate in general. For this reason, a Randomizer is required. Randomizer creates a random convex combination of probability so that one can take an arbitrary point inside the Element (simplex). Randomizer utilizes Mersenne twister and uniform distribution at the more fundamental level and can be seeded by the clock provided by chrono. The volume of the Element needs to be calculated at every integration and this is the reason why VolumeCalculator is a static object shared by all Elements.

Solver includes a pointer to the mesh, PDE and the boundary conditions. It uses MeshFiller to fill the initial Mesh covering the domain by creating a large enough box. Refine method accesses mesh pointer's refine and reset_indices method. The resulting linear matrix equation of the weak pde problem is of the form A_inner x c_inner == f - b, where A_inner is a sparse matrix of the dimensions (max_inner_index +1) x (max_inner_index +1) and other symbols are (max_inner_index +1)-dimensional vectors. A and b can be calculated elementwise by the PDE. In order to parallelize the calculation, we cut the computation of both A and f in smaller steps, iterating over part the mesh from start_ind to the stop_index. For example, method get_sparse_stiffness_matrix_async uses get_sparse_stiffness_matrix_part to divide the calculation. Now every part-method can be run asynchronously and later on they can be combined to yield the partial results and combine them. Number of parts corresponds to the number of CPU cores and we gain speed a advantage with roughly the same factor. Same technique is used for calculating vector f.

In addition to inner coeffs for c, the outer coeffs need to be set by choosing them as values of the boundary function (val). Furthermore, we need to calculate vector b, which results from moving the outer indices part of original bilinear product comprising of all indices to the right hand side. Essentially, this reduces to a product of the boundary part of the stiffness matrix A_outer and the outer coeffs c_outer, both of which has already been calculated. Once c_inner is solved by conjugate derivate method (from Eigen) we combine c_inner and c_outer to c_tot which gives the total solution as a linear combination of the base SimplexFunctions. The solution can be easily visualized by the fact that at the vertices, the base functions have the value 1. For this Solver has methods to both strore the grid and store the solution points either as matrix or as Mathematica arrays, depending on what OutputType is given for save methods. For accessing the mesh and combining solution vector c_tot with the grid points Solver creates DAOs of the right sort and iterates over the mesh with these.

3. Comparision with Fenics

Fenics is a widely used professional FEM framework solving PDEs in arbitrary domains. The python script testFenics.py defines a similar analytically solvable 2-D Poisson equation and solves it using regular mesh. Only the solving parts were compared, Fenics builds the mesh quite slowly since it is accessed through python modules. In order to test the fastest version of PDE Solver Monte Carlo integration was replaced by one point approximation of the integral f. Even then, Fenics beats it by a factor of 2 in speed/per mesh element once we have couple of hundred of elements and by a factor 10 when we have 10 k elements. Accuracy is roughly in the same ballpark for both methods at first, but even here Fenics beats PDE Solver by a factor of 20 when we hit 10k elements. Using Monte Carlo methods significantly improves the accuracy but happens at the cost of the computation speed. When the mesh size is around 400 elements, the computation speed is ca 0.2 ms / Element. When Monte Carlo integration is used, the speed drops to 1 ms / Element. At the same time, accuracy improves from 6x10^-6 to 3x10^-6. On coarser meshes, PDE Solver's accuracy beats Fenics when MC integration (with n == 50) is used.
