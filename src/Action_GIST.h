#ifndef INC_ACTION_GIST_H
#define INC_ACTION_GIST_H
#include "Action.h"
#include "ImageOption.h"
#include "Timer.h"
#include "EwaldOptions.h"
#ifdef CUDA
#include "cuda_kernels/GistCudaSetup.cuh"
#endif
#ifdef LIBPME
#include "GIST_PME.h"
#endif
class DataSet_3D;
class DataSet_MatrixFlt;
class DataSet_GridFlt;
class DataSet_GridDbl;



/// Class for applying Grid Inhomogenous Solvation Theory
/** \author Daniel R. Roe
  */
class Action_GIST : public Action {
  public:
    Action_GIST();
    #ifdef CUDA
    ~Action_GIST() {delete[] this->solvent_;}
    #endif
    DispatchObject* Alloc() const { return (DispatchObject*)new Action_GIST(); }
    void Help() const;
  private:
    Action::RetType Init(ArgList&, ActionInit&, int);
    Action::RetType Setup(ActionSetup&);
    Action::RetType DoAction(int, ActionFrame&);
    void Print();

    class Site; /// Potential hydrogen bond site. Can be either donor or donor/acceptor.

    typedef std::vector<float> Farray;
    typedef std::vector<int> Iarray;
    typedef std::vector<Farray> Xarray;
    typedef std::vector<double> Darray;
    typedef std::vector<Site> Sarray;

    inline void TransEntropy(float,float,float,float,float,float,float,int,double&,double&) const;
    static inline void Ecalc(double, double, double, NonbondType const&, double&, double&);
    void NonbondEnergy_pme(Frame const&);
    void NonbondEnergy(Frame const&, Topology const&);
    void Order(Frame const&);
    void SumEVV();
    void CalcAvgVoxelEnergy_PME(double, DataSet_GridFlt&, DataSet_GridFlt&, Farray&) const;
    void CalcAvgVoxelEnergy(double, DataSet_GridFlt&, DataSet_GridFlt&, DataSet_GridFlt&, DataSet_GridFlt&, Farray&, Farray&,
                            DataSet_GridDbl&, DataSet_GridFlt&, Farray&);

    
    /// functuion for hbond analysis-------
    inline double Angle(const double*, const double*, const double*, Box const&) const;
    bool EvalAngle(Site const&,const double*,int,const double*,
                        Frame const&); // return ture if the distance and angle criteria of hbond is met,
    void Hbond(Frame const&); // hbond calcution to update the voxel-wise hbond data
    // IsFON()
        /** Default criterion for being a hydrogen bond donor/acceptor. */
    inline bool IsFON(Atom const& atm) {
        return (atm.Element() == Atom::FLUORINE ||
        atm.Element() == Atom::OXYGEN ||
        atm.Element() == Atom::NITROGEN);
        }
    //-----------------------------------------
    
    int debug_;      ///< Action debug level
    int numthreads_; ///< Number of OpenMP threads
#ifdef CUDA
    // Additional data for GPU calculation

    std::vector<float> lJParamsA_;
    std::vector<float> lJParamsB_;
    std::vector<float> charges_;
    std::vector<int> atomTypes_;
    std::vector<int> NBIndex_;
    std::vector<int> molecule_;

    unsigned int numberAtoms_;
    int numberAtomTypes_;
    int headAtomType_;
    bool *solvent_; // TODO just use atomIsSolute_ array

    // Arrays on GPU
    int *NBindex_c_;
    void *molecule_c_;
    void *paramsLJ_c_;
    float *max_c_;
    float *min_c_;
    float *result_w_c_;
    float *result_s_c_;
    int *result_O_c_;
    int *result_N_c_;

    // CUDA only functions
    void freeGPUMemory(void);
    void copyToGPU(void);
    void NonbondCuda(ActionFrame);

#endif

#   ifdef LIBPME
    GIST_PME gistPme_;     ///< Holds GIST PME functionality
#   endif
    bool usePme_;          ///< If true, try to use GIST PME
    EwaldOptions pmeOpts_; ///< Hold PME options for GIST PME

    static const Vec3 x_lab_;
    static const Vec3 y_lab_;
    static const Vec3 z_lab_;
    static const double maxD_;
    static const double QFAC_;
    static const int OFF_GRID_; ///< Value in atom_voxel_ that indicates atom is off the grid

    double gridspacing_;
    Vec3 gridcntr_;
    Vec3 griddim_;

    // NOTE: '*' = Updated in DoAction(). '+' = Updated in Setup().
    ImageOption imageOpt_;  ///< Used to determine if imaging should be used.*
    // GIST float grid datasets
    DataSet_3D* gO_;        ///< Solvent oxygen density
    DataSet_3D* gH_;        ///< Solvent hydrogen density
    DataSet_3D* Esw_;       ///< Solute-water energy
    DataSet_3D* Eww_;       ///< Water-water energy
    DataSet_3D* dTStrans_;  ///< Solvent translation entropy
    DataSet_3D* dTSorient_; ///< Solvent orentational entropy
    DataSet_3D* dTSsix_;
    DataSet_3D* neighbor_norm_;
    DataSet_3D* Eww_neighbor_; ///< water-water energy per neighbor
    DataSet_3D* Esw_neighbor_; ///< water-solute energy per solute neighbor
    DataSet_3D* dipole_; // pol
    // GIST double grid datasets
    DataSet_3D* order_norm_; // qtet
    DataSet_3D* dipolex_;    ///< Water dipole (X)*
    DataSet_3D* dipoley_;    ///< Water dipole (Y)*
    DataSet_3D* dipolez_;    ///< Water dipole (Z)*
    // PME GIST double grid datasets
    DataSet_3D* PME_;           ///< The PME nonbond interaction( charge-charge + vdw) cal for water
    DataSet_3D* U_PME_;         ///< The PME nonbond energy for solute atoms
    // GIST matrix datasets
    DataSet_MatrixFlt* ww_Eij_; ///< Water-water interaction energy matrix.*
    
    // hbond grid dataset
    DataSet_3D* sw_Don_; //-> For each voxel, number of solute-water hydrogen bond, in which water in this voxel acts as donor
    DataSet_3D* sw_Acc_; //-> For each voxel, number of solute-water hydrogen bond, in which water in this voxel acts as acceptor
    DataSet_3D* ww_Don_; //-> For each voxel, number of water-water hydrogen bond, in which water in this voxel acts as donor 
    DataSet_3D* ww_Acc_; //-> For each voxel, number of water-water hydrogen bond, in which water in this voxel acts as acceptor

    //Iarray mol_nums_;     ///< Absolute molecule number of each solvent molecule.+ //TODO needed?
    Iarray O_idxs_;         ///< Oxygen atom indices for each solvent molecule.+
    Iarray OnGrid_idxs_;    ///< Indices for each water atom on the grid.*
    Iarray atom_voxel_;     ///< Absolute grid voxel for each atom (OFF_GRID_ if atom not on grid).*
    Iarray A_idxs_;         ///< Atom indices for each solute and solvent atom (energy calc only).+
    std::vector<bool> atomIsSolute_; ///< True if atom is solute.+
    std::vector<bool> atomIsSolventO_; ///< True if atom is sovent O.+
    Iarray U_idxs_;         ///< Atom indices for solute atoms only.+
    Iarray U_onGrid_idxs_;  ///< Indices for each solute atom on the grid.*
    Iarray N_waters_;       ///< Number of waters (oxygen atoms) in each voxel.*
    Iarray N_solute_atoms_; ///< Number of solute atoms in each voxel.*
    Iarray N_hydrogens_;    ///< Number of hydrogen atoms in each voxel.*
#   ifdef _OPENMP
    std::vector<Iarray> EIJ_V1_; ///< Hold any interaction energy voxel 1 each frame.*
    std::vector<Iarray> EIJ_V2_; ///< Hold any interaction energy voxel 2 each frame.*
#   endif

    std::vector<Farray> neighbor_; ///< Number of water neighbors within 3.5 Ang.*
    std::vector<Farray> s_neighbor_; ///< Number of solute atom neighbnor within 3.5 Ang.*
    
#   ifdef _OPENMP
    std::vector<Farray> EIJ_EN_;   ///< Hold any interaction energies each frame.*
#   endif

    Xarray voxel_xyz_; ///< Coords for all waters in each voxel.*
    Xarray voxel_Q_;   ///< w4, x4, y4, z4 for all waters in each voxel.*

    Darray OnGrid_XYZ_;             ///< XYZ coordinates for on-grid waters.*
    std::vector<Darray> E_UV_VDW_;  ///< Solute-solvent van der Waals energy for each voxel.*
    std::vector<Darray> E_UV_Elec_; ///< Solute-solvent electrostatic energy for each voxel.*
    std::vector<Darray> E_VV_VDW_;  ///< Solvent-solvent van der Waals energy for each voxel.*
    std::vector<Darray> E_VV_Elec_; ///< Solvent-solvent electrostatic energy for each voxel.*
    std::vector<Darray> E_VV_neighbor_; /// average water-water energy per neighbor for each voxel.*
    std::vector<Darray> E_UV_neighbor_; /// average solute-water energy per solute neighbor for each voxel.*

    //Add for hydrogen bond analysis-------
    Darray Nsw_don_; ///< Number of solute-water hbond, in which water acts as donor.*
    Darray Nsw_acc_; ///< Number of solute-water hbond, in which water acts as acceptor.*
    Darray Nww_don_; ///< Number of water-water hbond, in which water acts as donor.*
    Darray Nww_acc_; ///< Number of water-water hbond, in which water acts as acceptor.*

    AtomMask AcceptorMask_; /// Acceptor from solute F/O/N
    AtomMask Mask_;  /// All atom in the system
    AtomMask SolventDonorMask_; /// water residues
    AtomMask SolventAcceptorMask_; /// oxgen atoms in water molecules

    
    Sarray Both_;         ///< Array of donor sites that can also be acceptors, will be apppended with donor-only solute atom after bothEnd_
    Iarray Acceptor_;     ///< Array of acceptor-only solute atom indices
    Sarray SolventSites_; ///< Array of solvent donor/acceptor sites
    
    // gemotric criteria for h bond, same as in Action_HydrogenBond
    double dcut2_; // the distance of A and D atom cutoff squared for Hbod, default 3 Angstrom.
    double acut_;  // the angle cutoff for A-H-D, default 135 degree
    unsigned int bothEnd_;   ///< Index in Both_ where donor-only sites begin
    
    ///-----------------------------------------


    // PME energy terms
    Darray E_pme_;     ///< Total nonbond interaction energy(VDW + electrostatic) calculated by PME for water TODO grid?
    Darray U_E_pme_;   ///< Total nonbond interaction energy(VDW + Elec) calculated by PME for solute TODO grid?

    Vec3 G_max_; ///< Grid max + 1.5 Ang.

    // Timing data
    Timer gist_init_;
    Timer gist_setup_;
    Timer gist_action_;
    Timer gist_print_;
    Timer gist_grid_;
    Timer gist_nonbond_;
    Timer gist_nonbond_dist_;
    Timer gist_nonbond_UV_;
    Timer gist_nonbond_VV_;
    Timer gist_nonbond_OV_;
    Timer gist_euler_;
    Timer gist_dipole_;
    Timer gist_order_;
    Timer gist_hbond_; 

    Topology* CurrentParm_;    ///< Current topology, for energy calc.
    CpptrajFile* datafile_;    ///< GIST output
    CpptrajFile* eijfile_;     ///< Eij matrix output
    CpptrajFile* infofile_;    ///< GIST info
    std::string prefix_;       ///< Output file name prefix
    TextFormat fltFmt_;        ///< Output file format for floating point values
    TextFormat intFmt_;        ///< Output file format for integer values.
    Darray Q_;                 ///< Solvent molecule charges (for dipole calc)
    double BULK_DENS_;         ///< Bulk water density
    double temperature_;       ///< Temperature
    double NeighborCut2_;      ///< Cutoff for determining water neighbors (squared).
//    double system_potential_energy_; ///< the emsemble average potential energy ( Eelec + Vdw ) for the frames (pme only)
//    double solute_potential_energy_; ///< the ensemble average potential energy on solute atoms (pme only)
    unsigned int MAX_GRID_PT_; ///< Max number of grid points (voxels).
    unsigned int NSOLVENT_;    ///< Number of solvent molecules.
    unsigned int N_ON_GRID_;   ///< Number of water atoms on the grid.*
    unsigned int nMolAtoms_;   ///< Number of atoms in a water molecule.+
    int NFRAME_;               ///< Total # frames analyzed
    int max_nwat_;             ///< Max number of waters in any voxel
    bool doOrder_;             ///< If true do the order calc
    bool doEij_;               ///< If true do the i-j energy calc
    bool skipE_;               ///< If true skip the nonbond energy calc
    bool includeIons_;         ///< If true include ions in solute region.
    bool skipS_;               ///< If true does not calculate entropy
    bool hbond_;               ///< If ture do the hbind analysis, defualt is false
};
#endif
class Action_GIST::Site {
  public:
    Site() : idx_(-1) {}
    /// Solute site - heavy atom, hydrogen atom
    Site(int d, int h) : hlist_(1,h), idx_(d) {}
    /// Solute site - heavy atom, list of hydrogen atoms
    Site(int d, Iarray const& H) : hlist_(H), idx_(d) {}
    /// \return heavy atom index
    int Idx() const { return idx_; }
    /// \return number of hydrogen indices
    unsigned int n_hydrogens()      const { return hlist_.size(); }
    /// \return true if site is an ion (D atom == H atom)
    bool IsIon() const { return (hlist_.size()==1 && hlist_[0] == idx_); }
    /// \return iterator to beginning of hydrogen indices
    Iarray::const_iterator Hbegin() const { return hlist_.begin(); }
    /// \return iterator to end of hydrogen indices
    Iarray::const_iterator Hend()   const { return hlist_.end(); }
  private:
    Iarray hlist_; ///< List of hydrogen indices
    int idx_;      ///< Heavy atom index
};

