# Contributing to FiXPro

Thank you for your interest in contributing to FiXPro!

## Code of Conduct

Please be respectful and constructive in all interactions. We follow the [Contributor Covenant](https://www.contributor-covenant.org/).

## How to Contribute

### Reporting Bugs

1. Search existing issues first
2. Use the bug report template
3. Include:
   - Clear description
   - Steps to reproduce
   - Expected vs actual behavior
   - System information
   - Log files if applicable

### Suggesting Features

1. Search existing issues first
2. Use the feature request template
3. Explain the use case
4. Provide code examples if possible

### Pull Requests

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes
4. Run tests: `pytest tests/`
5. Commit with clear messages
6. Push and create PR

## Development Setup

### Prerequisites

- Python 3.8+
- PlatformIO
- Git

### Quick Start

```bash
# Clone
git clone https://github.com/MYMDO/fixpro.git
cd fixpro

# Install dev dependencies
pip install -e ".[dev]"

# Build firmware
cd firmware/platformio
pio run

# Run tests
pytest tests/
```

## Code Style

### Python

- Follow PEP 8
- Use type hints
- Max line length: 100
- Run: `black . && flake8 . && mypy .`

### C/C++

- Follow K&R style
- Use meaningful names
- Comment complex logic
- Max line length: 120

### JavaScript

- Use ES6+ features
- Use `const` and `let`
- Run: `npm run lint`

## Project Structure

```
FiXPro/
├── firmware/           # Embedded firmware
│   ├── platformio/    # PlatformIO project
│   └── src/           # Pico SDK project
├── host/              # Host tools
│   └── cli/           # Python CLI
├── tests/             # Test files
├── docs/              # Documentation
└── chipdb/           # Chip databases
```

## Testing

### Unit Tests

```bash
pytest tests/protocol/
```

### Integration Tests

```bash
pytest tests/integration/
```

### Hardware Tests

```bash
pytest tests/hardware/
```

## Commit Messages

Use conventional commits:

```
feat: add new feature
fix: fix bug
docs: update documentation
style: formatting changes
refactor: code refactoring
test: add tests
chore: maintenance tasks
```

## License

By contributing, you agree that your contributions will be licensed under GPL-3.0.
