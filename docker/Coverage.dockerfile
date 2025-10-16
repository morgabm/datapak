    # Use a base image with a C++ toolchain
    FROM ubuntu:latest

    # Install necessary tools
    RUN apt-get update && apt-get install -y \
        build-essential \
        cmake \
        git \
        lcov \
        g++ \
        libpthread-stubs0-dev # Example for a common dependency

    # Set working directory
    WORKDIR /app

    # Copy your project files into the container
    # COPY . /app

    # Build your application with coverage flags
    # For GCC: -fprofile-arcs -ftest-coverage
    # RUN mkdir build_coverage && cd build_coverage && \
    #     cmake -DCMAKE_CXX_FLAGS="-fprofile-arcs -ftest-coverage" -DENABLE_COVERAGE .. && \
    #     make coverage-summary

    # # Run your tests (this generates coverage data)
    # RUN cd build && ./your_test_executable

    # # Generate coverage report using lcov
    # RUN cd build && \
    #     lcov --capture --directory . --output-file coverage.info && \
    #     genhtml coverage.info --output-directory coverage_html

    # Optional: Expose a port if you want to serve the HTML report
    # EXPOSE 8080
    # CMD ["python3", "-m", "http.server", "8080", "--directory", "build/coverage_html"]