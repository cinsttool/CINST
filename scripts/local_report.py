
sunflow_reports = [
(912624, 'org/sunflow/core/Ray', 106, 'org/sunflow/core/Ray'),
(436394, 'org/sunflow/core/gi/InstantGI', 86, 'org/sunflow/core/Ray'),
(268357, 'org/sunflow/image/Color', 333, 'org/sunflow/image/Color'),
(44376, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 200, 'org/sunflow/math/Vector3'),
(44355, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 206, 'org/sunflow/math/Vector3'),
(38703, 'org/sunflow/math/Solvers', 23, '[double'),
(36471, 'org/sunflow/core/primitive/TriangleMesh', 498, 'org/sunflow/math/Point3'),
(6622, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 116, 'org/sunflow/math/Vector3'),
(5991, 'org/sunflow/math/Point3', 105, 'org/sunflow/math/Vector3'),
(5989, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 120, 'org/sunflow/math/Vector3'),
(5962, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 121, 'org/sunflow/math/Vector3'),
(5748, 'org/sunflow/image/Color', 272, 'org/sunflow/image/Color'),
(5563, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 144, 'org/sunflow/math/Vector3'),
(5551, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 129, 'org/sunflow/math/Vector3'),
(5541, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 150, 'org/sunflow/math/Vector3'),
(5529, 'org/sunflow/core/light/TriangleMeshLight$TriangleLight', 138, 'org/sunflow/math/Vector3'),
(4083, 'org/sunflow/image/Color', 18, 'org/sunflow/image/Color'),
(3333, 'org/sunflow/core/accel/KDTree', 446, '[int'),
(3329, 'org/sunflow/core/accel/KDTree', 451, '[float'),
(3324, 'org/sunflow/core/accel/KDTree', 452, '[float'),
]

all_count = 2385222



print(f'Count of all object allocation: {all_count}')
print(f'Top 10 candidates for redundant stack-bound objects:')
print('Count, Ratio for all, Position, Class')
i = 0
for line in sunflow_reports:
    print(f'{line[0]}, {line[0]/all_count}, {line[1]}:{line[2]}, {line[-1]}')

