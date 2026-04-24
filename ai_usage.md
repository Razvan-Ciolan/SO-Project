# AI Usage Report - Phase 1

**Student:** Razvan-Ciolan
**Project:** City Infrastructure Management System (SO)
**AI Tool Used:** Gemini 3 Flash

## 1. Scope of AI Assistance
For the first phase of the project, I used an AI assistant specifically to help with the implementation of the filtering system. The AI was used to generate two core utility functions:
- `parse_condition`: To split the filter string (e.g., "severity:>=:2") into components.
- `match_condition`: To evaluate individual reports against these components.

## 2. Integration and Manual Adjustments
While the AI provided the logic for field comparison, I had to manually integrate these functions into the Unix file-handling architecture of the project.

**Key adjustments made to the AI output:**
- **Data Type Correction:** The AI initially used standard integer comparisons. I updated the logic to handle `time_t` for timestamps using `atol()` to ensure correct evaluation of dates.
- **I/O Logic:** I wrote the `filter_reports` function from scratch to handle the sequential reading of the binary file using Unix system calls (`open`, `read`, `close`) and implemented the logic to ensure that multiple conditions are treated as an **AND** operation.

## 3. Technical Insights Gained
- **String Parsing:** I learned how to use `sscanf` with scanning sets (`%[^:]`) as a more robust alternative to `strtok` for parsing formatted strings without modifying the original memory.
- **Binary Data Management:** The process helped me understand how to perform complex queries on binary files by iterating through records and applying conditional filters in real-time.
- **Git Workflow:** Beyond coding, resolving conflicts between local branches and GitHub’s privacy settings was a significant learning point regarding professional version control.
