# Case Studies

This page summarizes real optimizations discovered with CINST and validated on real Java applications.

## Summary of optimization outcomes

CINST-reported container inefficiencies were fixed with small code changes and produced meaningful end-to-end speedups. In the reported experience:

- optimizations preserved semantics across inputs and passed project tests,
- each issue was resolved within 1-2 days by a junior graduate student,
- multiple fixes were submitted upstream, and over half were merged.

Upstream PRs:

- Apache Lucene: <https://github.com/apache/lucene/pull/13254>
- Apache PDFBox: <https://github.com/apache/pdfbox/pull/188>
- BioJava: <https://github.com/biojava/biojava/pull/1089>
- FASTJSON2: <https://github.com/alibaba/fastjson2/pull/2385>
- ZXing: <https://github.com/zxing/zxing/pull/1782>