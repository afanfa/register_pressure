#include "hip/hip_runtime.h"

__global__ __launch_bounds__(256,1) void kernel (const double * __restrict__ phi, const double * __restrict__ laplacian_phi,
						 const double * __restrict__ grad_phi_x, const double * __restrict__ grad_phi_y, const double * __restrict__ grad_phi_z,
						 double * __restrict__ f0, double * __restrict__ f1, double * __restrict__ f2, double * __restrict__ f3, double * __restrict__ f4,
						 double * __restrict__ f5, double * __restrict__ f6,
						 double * __restrict__ g0, double * __restrict__ g1, double * __restrict__ g2, double * __restrict__ g3, double * __restrict__ g4,
						 double * __restrict__ g5, double * __restrict__ g6, double* __restrict__ g7, double * __restrict__ g8, double * __restrict__ g9,
						 double * __restrict__ g10, double * __restrict__ g11, double * __restrict__ g12, double * __restrict__ g13, double * __restrict__ g14,
						 double * __restrict__ g15, double * __restrict__ g16, double * __restrict__ g17, double * __restrict__ g18,
						 const int nx, const int ny, const int nz, const int ldx, const int ldy, const int current, const int next,
						 const double k, const double alpha, const double phi2, const double gamma,
						 const double itauphi, const double itauphi1, const double ieta,
						 const double itaurho, const double grav,
						 const double eg1, const double eg2, const double eg0, const double egc0, const double egc1, const double egc2)
{
  const int i = (threadIdx.x + blockIdx.x * blockDim.x);
  const int j = (threadIdx.y + blockIdx.y * blockDim.y);
  const int z = (threadIdx.z + blockIdx.z * blockDim.z);

  //__shared__ double irho_s[256];
  double tmp1, tmp2;

  if(i <= nx && j <= ny && z <= nz)
    {
      const int m = i + ldx * (j + ldy * z);
      const int current_pos = m + current;
      
      const double current_phi = phi[m];
      const double current_phi_2 = current_phi * current_phi;

      const double rho = g0[m] + g1[current_pos] + g2[current_pos] + g3[current_pos] + g4[current_pos] +
	g5[current_pos] + g6[current_pos] + g7[current_pos] + g8[current_pos]  + g9[current_pos] +
	g10[current_pos] + g11[current_pos] + g12[current_pos] + g13[current_pos] + g14[current_pos] +
	g15[current_pos] + g16[current_pos] + g17[current_pos] + g18[current_pos];
      
      const double irho = 1.0/rho;

      const double mu_phi = alpha * current_phi * ( current_phi_2 - phi2 ) - k * laplacian_phi[m];

      const double fx = mu_phi * grad_phi_x[m];

      const double ux = ( g1[current_pos] - g2[current_pos] + g7[current_pos] - g8[current_pos] + g9[current_pos] -
			  g10[current_pos] + g11[current_pos] - g12[current_pos] + g13[current_pos] - g14[current_pos] +
			  0.50 * fx ) * irho;
      
      const double fy = mu_phi * grad_phi_y[m];
      
      const double uy = ( g3[current_pos] - g4[current_pos] + g7[current_pos] - g8[current_pos] - g9[current_pos] +
			  g10[current_pos] + g15[current_pos] - g16[current_pos] + g17[current_pos] - g18[current_pos] +
			  0.50 * fy ) * irho;

      const double fz = mu_phi * grad_phi_z[m];
      
      const double uz = ( g5[current_pos] - g6[current_pos] + g11[current_pos] - g12[current_pos] - g13[current_pos] +
			  g14[current_pos] + g15[current_pos] - g16[current_pos] - g17[current_pos] + g18[current_pos] +
			  0.50 * fz ) * irho;
      
      const double af = 0.50 * gamma * mu_phi * itauphi;
      const double cf = itauphi * ieta * current_phi;

      f0[m] = itauphi1 * f0[m] + -3.0 * gamma * mu_phi * itauphi + itauphi * current_phi;
      
      f1[current_pos] = itauphi1 * f1[current_pos] + af + cf * ux;
      f2[current_pos] = itauphi1 * f2[current_pos] + af - cf * ux;
      f3[current_pos] = itauphi1 * f3[current_pos] + af + cf * uy;
      f4[current_pos] = itauphi1 * f4[current_pos] + af - cf * uy;
      f5[current_pos] = itauphi1 * f5[current_pos] + af + cf * uz;
      f6[current_pos] = itauphi1 * f6[current_pos] + af - cf * uz;

      const double ag = 3.0 * current_phi * mu_phi + rho;
      const double v = 1.50 * ( ux*ux + uy*uy + uz*uz ); //closer to first use
      const double uf = ux * fx + uy * fy + uz * fz;
      
      g0[m] = itaurho * g0[m] + eg0 * ( (rho - 6.0 * current_phi * mu_phi) - rho*v ) - egc0*uf;

      const double eg1ag = eg1 * ag; //closer to first use
      const double eg1rho = eg1 * rho;
      
      tmp1 = eg1ag + eg1rho*( 0.50*ux*ux - v ) + egc1*( ux*fx - uf );
      tmp2 = eg1rho*ux + egc1*fx;
      
      g1[m+next + 1] = itaurho * g1[current_pos] + tmp1 + tmp2;
      g2[m+next - 1] = itaurho * g2[current_pos] + tmp1 - tmp2;
      
      tmp1 = eg1ag + eg1rho*( 0.50 * uy * uy - v ) + egc1 * ( uy * fy - uf );
      tmp2 = eg1rho * uy + egc1 * fy;
      
      g3[m+next + ldx] = itaurho * g3[current_pos] + tmp1 + tmp2;
      g4[m+next - ldx] = itaurho * g4[current_pos] + tmp1 - tmp2;
      
      tmp1 = eg1ag + eg1rho*( 0.50 * uz * uz - v ) + egc1 * ( uz * fz - uf );
      tmp2 = eg1rho * uz + egc1 * fz;
      
      g5[m+next + ldx*ldy] = itaurho * g5[current_pos] + tmp1 + tmp2;
      g6[m+next - ldx*ldy] = itaurho * g6[current_pos] + tmp1 - tmp2;

      const double eg2ag = eg2 * ag; //closer to fist use
      const double eg2rho = eg2 * rho;
      
      tmp1 = eg2ag + eg2rho * ( 0.50 * ( ux + uy ) * ( ux + uy ) - v ) +
	egc2 * ( ( ux + uy ) * ( fx + fy ) - uf );
      
      tmp2 = eg2rho * ( ux + uy ) + egc2 * ( fx + fy );
      
      g7[m+next + 1 + ldx] = itaurho * g7[current_pos] + tmp1 + tmp2;
      g8[m+next - 1 - ldx] = itaurho * g8[current_pos] + tmp1 - tmp2;
      
      tmp1 = eg2ag + eg2rho * ( 0.50 * ( ux - uy ) * ( ux - uy ) - v ) +
	egc2 * ( ( ux - uy )*( fx - fy ) - uf );
      tmp2 = eg2rho * ( ux - uy ) + egc2 * ( fx - fy );
      
      g9[m+next + 1 - ldx]  = itaurho * g9[current_pos]  + tmp1 + tmp2;
      g10[m+next - 1 + ldx] = itaurho * g10[current_pos] + tmp1 - tmp2;
      
      tmp1 = eg2ag + eg2rho * ( 0.50 * ( ux + uz ) * ( ux + uz ) - v ) +
	egc2 * ( ( ux + uz ) * ( fx + fz ) - uf );
      tmp2 = eg2rho * ( ux + uz ) + egc2 * ( fx + fz );
      
      g11[m+next + 1 + ldx*ldy] = itaurho * g11[current_pos] + tmp1 + tmp2;
      g12[m+next - 1 - ldx*ldy] = itaurho * g12[current_pos] + tmp1 - tmp2;
      
      tmp1 = eg2ag + eg2rho * ( 0.50 * ( ux - uz ) * ( ux - uz ) - v ) +
	egc2 * ( ( ux - uz ) * ( fx - fz ) - uf );
      tmp2 = eg2rho * ( ux - uz ) + egc2 * ( fx - fz );
      
      g13[m+next + 1 - ldx*ldy] = itaurho * g13[current_pos] + tmp1 + tmp2;
      g14[m+next - 1 + ldx*ldy] = itaurho * g14[current_pos] + tmp1 - tmp2;
      
      tmp1 = eg2ag + eg2rho * ( 0.50 * ( uy + uz ) * ( uy + uz ) - v ) +
	egc2 * ( ( uy + uz ) * ( fy + fz ) - uf );
      tmp2 = eg2rho * ( uy + uz ) + egc2 * ( fy + fz );
      
      g15[m+next + ldx + ldx*ldy] = itaurho * g15[current_pos] + tmp1 + tmp2;
      g16[m+next - ldx - ldx*ldy] = itaurho * g16[current_pos] + tmp1 - tmp2;

      tmp1 = eg2ag + eg2rho * ( 0.50 * ( uy - uz ) * ( uy - uz ) - v ) +
	egc2 * ( ( uy - uz ) * ( fy - fz ) - uf );
      tmp2 = eg2rho * ( uy - uz ) + egc2 * ( fy - fz );
      
      g17[m+next + ldx - ldx*ldy] = itaurho * g17[current_pos] + tmp1 + tmp2;
      g18[m+next - ldx + ldx*ldy] = itaurho * g18[current_pos] + tmp1 - tmp2;
      
    }
  }