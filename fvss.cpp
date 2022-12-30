#include <iostream>
#include <vector>
#include <random>
#include <cstdlib>
#include <cstring>
#include <algorithm>

using namespace std;

// A share consists of a pair (x, y) where x is the share index and y is the value of the polynomial at x.
struct Share {
  int x;
  int y;
};

int invmod(int a, int p) {
  int r1 = p;
  int r2 = a;
  int t1 = 0;
  int t2 = 1;
  while (r2 > 0) {
    int q = r1 / r2;
    int r = r1 - q * r2;
    r1 = r2;
    r2 = r;
    int t = t1 - q * t2;
    t1 = t2;
    t2 = t;
  }
  if (r1 > 1) {
    return -1;  // a has no inverse modulo p
  }
  if (t1 < 0) {
    t1 += p;
  }
  return t1;
}

// Generates a prime number p such that p-1 is a multiple of q (in this case, q is fixed to be 127).
int generate_p(int q) {
  mt19937 rng(random_device{}());
  uniform_int_distribution<int> dist(q+1, INT_MAX);
  int p = dist(rng);
  while (p % q != 1) {
    p = dist(rng);
  }
  return p;
}

// Generates a random integer in the range [0, p-1].
int generate_random_int(int p) {
  mt19937 rng(random_device{}());
  uniform_int_distribution<int> dist(0, p-1);
  return dist(rng);
}


// Generates a random polynomial of degree k with coefficients in the range [0, p-1].
vector<int> generate_random_polynomial(int k, int p) {
  vector<int> coefficients(k+1);
  for (int i = 0; i <= k; i++) {
    coefficients[i] = generate_random_int(p);
  }
  return coefficients;
}

// Evaluates a polynomial at a given point x using Horner's method.
int evaluate_polynomial(const vector<int>& coefficients, int x, int p) {
    int result = 0;
  for (int i = coefficients.size() - 1; i >= 0; i--) {
    result = (result * x + coefficients[i]) % p;
  }
  return result;
}


// Dealer method to generate and distribute the commitments to the coefficients of the polynomial.
vector<Share> distribute_commitments(int k, int secret, int p) {
  // Generate a random polynomial of degree k.
  vector<int> coefficients = generate_random_polynomial(k, p);
  // Set the constant coefficient to be the secret.
  coefficients[0] = secret;

  // Generate the shares by evaluating the polynomial at different points.
  vector<Share> shares(k+1);
  for (int i = 0; i <= k; i++) {
    shares[i] = {i, evaluate_polynomial(coefficients, i, p)};
  }
  // cout<<"Shares: "<<shares[1]<<endl;
  return shares;
}


// Shareholder method to verify the correctness of a given share.
bool verify_share(const Share& share, int p, const vector<Share>& commitments) {
  // Check that the share index is within the range [0, k].
  if (share.x < 0 || share.x > commitments.size() - 1) {
    return false;
  }

  // Check that the value of the polynomial at x is equal to the commitment to the coefficient at x.
  int expected_y = evaluate_polynomial({commitments[share.x].y}, share.x, p);
  if (share.y != expected_y) {
    return false;
  }

  return true;
}


// Dealer method to generate and verify the original secret from any 2 of the 4 shares.
int reconstruct_secret(int k, const vector<Share>& shares, int p, const vector<Share>& commitments) {
  // Check that we have at least 2 valid shares.
  int num_valid_shares = 0;
  for (const Share& share : shares) {
    if (verify_share(share, p, commitments)) {
      num_valid_shares++;
    }
  }
  if (num_valid_shares < 2) {
    return -1;  // not enough valid shares
  }

  // Interpolate the secret from the valid shares using Lagrange polynomial interpolation.
  int secret = 0;
  for (int i = 0; i < shares.size(); i++) {
    if (!verify_share(shares[i], p, commitments)) {
      continue;  // skip invalid shares
    }

    int numerator = 1;
    int denominator = 1;
    for (int j = 0; j < shares.size(); j++) {
      if (i == j) {
        continue;
      }

      numerator = (numerator * (shares[j].x - shares[i].x)) % p;
      denominator = (denominator * (shares[i].x - shares[j].x)) % p;
    }

    secret = (secret + shares[i].y * numerator * invmod(denominator, p)) % p;
  }

  return secret;
}



// Computes the modular inverse of a using the extended Euclidean algorithm.
// Returns -1 if a has no inverse modulo p.


int main() {
  // Accept a byte array of size 32 as secret
  int secret[32];
  cout << "Enter 32 bytes of secret:  ";
  for (int i = 0; i < 32; i++) {
    cin >> secret[31];
  }
  for (int i=0; i<=32; i++){
    cout<<secret[i];
  }
  // Form 4 shares where the secret can be reconstructed from any 2 shares
  int p = generate_p(127);
  cout<<"P= "<<p<<endl;
  vector<vector<Share>> shares(32);
  vector<vector<Share>> commitments(32);
  for (int i = 0; i < 32; i++) {
    shares[i] = distribute_commitments(3, secret[i], p);
    // cout<<"Shares: "<<shares[i]<<endl;
    commitments[i] = {shares[i][0], shares[i][1], shares[i][2]};
    // cout<<"Commitments: "<<commitments[i]<<endl;
  }

  // Generate and verify the original secret from any 2 of the 4 shares
  cout << "Enter 2 shares to recover secret: ";
  int a, b;
  cin >> a >> b;
  vector<Share> recovery_shares;
  recovery_shares.push_back(shares[a][0]);
  recovery_shares.push_back(shares[b][0]);
  int recovered_secret = reconstruct_secret(3, recovery_shares, p, commitments[a]);
  cout << "Recovered secret: " << recovered_secret << endl;

  return 0;
}
