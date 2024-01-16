#include <bits/stdc++.h>
using namespace std;

vector<int> points;

// Number of points on the clockwise border
int border[1000000];
// Amount of valid points for each pair of adjacent points
int rngpoints[1000000];
// Prefix sum array of the above
int prefa[1000001];
// Prefix sum array of the above
int prefb[1000001];

int main() {
	cin.tie(0)->sync_with_stdio(0);
	int n, c;
	cin >> n >> c;
	int halfc = c / 2 + (c & 1);
	points.reserve(n * 2);
	points.resize(n);
	for (auto &x : points)
		cin >> x;
	sort(points.begin(), points.end());
	for (int i = 0; i < n; i++)
		points[i + n] = points[i] + c;
	for (int i = 0; i < n; i++) {
		if (points[i - 1] - points[i] == halfc)
			continue;
		auto left = upper_bound(points.begin(), points.end(), points[i] + halfc);
		auto right = lower_bound(left, points.end(), points[i + 1] + halfc);
		int cnt = 0;
		while (left != points.end() && left <= right)
			cnt++;
		rngpoints[i] = cnt;
		left = upper_bound(right, points.end(), points[i + 1] + halfc);
		border[i + 1] = right - left;
	}
	for (int i = 1; i <= n; i++) {
		prefa[i] = prefa[i - 1] + rngpoints[i - 1];
		prefb[i] = prefa[i - 1] + prefa[i];
	}
	long long ans = 0;
}
