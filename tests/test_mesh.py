import json,math,unittest
from pathlib import Path

class ApprovedMesh(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data=json.loads((Path(__file__).resolve().parents[1]/'projection/approved-mesh.json').read_text())
        cls.p=cls.data['vertices'];cls.n=cls.data['size']
    def test_grid_and_boundary(self):
        self.assertEqual(self.n,49)
        self.assertEqual(len(self.p),self.n)
        for j,row in enumerate(self.p):
            self.assertEqual(len(row),self.n)
            for i,(x,y) in enumerate(row):
                self.assertTrue(math.isfinite(x) and math.isfinite(y))
                self.assertLessEqual(x*x+y*y,1+1e-12)
                if i in (0,self.n-1) or j in (0,self.n-1):
                    self.assertAlmostEqual(x*x+y*y,1,places=10)
    def test_no_fold_at_any_face_corner(self):
        # A bilinear face has an affine determinant; its corners bound the sign.
        for j in range(self.n-1):
            for i in range(self.n-1):
                p,q,r,s=self.p[j][i],self.p[j][i+1],self.p[j+1][i],self.p[j+1][i+1]
                for a,b in [(0,0),(0,1),(1,0),(1,1)]:
                    dx=[(1-b)*(q[k]-p[k])+b*(s[k]-r[k]) for k in range(2)]
                    dy=[(1-a)*(r[k]-p[k])+a*(s[k]-q[k]) for k in range(2)]
                    self.assertGreater(dx[0]*dy[1]-dx[1]*dy[0],0)
if __name__=='__main__':unittest.main()
