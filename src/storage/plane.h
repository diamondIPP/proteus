#ifndef PLANE_H
#define PLANE_H

#include <vector>
#include <string>

namespace Storage {
  
  class Hit;
  class Cluster;
  
  /*******************************************************************************
   * Plane class contains the hits and clusters for one sensor plane as well as
   * a plane number.
   */
  class Plane {
  protected:
    Plane(unsigned int planeNum);
    ~Plane() { ; }

  public:
    friend class Event;

    inline unsigned int getPlaneNum() const { return _planeNum; }
    inline unsigned int getNumHits() const { return _numHits; }
    inline unsigned int getNumClusters() const { return _numClusters; }
    inline unsigned int getNumIntercepts() const { return _numIntercepts; }

    Hit* getHit(unsigned int n) const;
    Cluster* getCluster(unsigned int n) const;
    std::pair<double, double> getIntercept(unsigned int n) const;
    
    void addIntercept(double posX, double posY);
    void print() const;
  
  protected:
    void addHit(Hit* hit);
    void addCluster(Cluster* cluster);
   
  private:
    unsigned int _planeNum;
    unsigned int _numHits;
    std::vector<Hit*> _hits;
    unsigned int _numClusters;
    std::vector<Cluster*> _clusters;
    unsigned int _numIntercepts;
    std::vector<std::pair<double, double>> _intercepts;  
  };  
}

#endif // PLANE_H
