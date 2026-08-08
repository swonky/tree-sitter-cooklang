use criterion::{black_box, criterion_group, criterion_main, Criterion};
use tree_sitter::Parser;

const TEST_RECIPE: &str = include_str!("./test_recipe.cook");
const FRONTMATTER_RECIPE: &str = include_str!("./frontmatter_test_recipe.cook");
const COMPLEX_TEST_RECIPE: &str = include_str!("./complex_test_recipe.cook");

fn parser() -> Parser {
    let mut parser = Parser::new();
    parser
        .set_language(&tree_sitter_cooklang::LANGUAGE.into())
        .unwrap();
    parser
}

fn canonical(c: &mut Criterion) {
    let mut group = c.benchmark_group("canonical");

    group.bench_with_input("parse-canonical", TEST_RECIPE, |b, input| {
        let mut parser = parser();
        b.iter(|| {
            black_box(parser.parse(black_box(input), None).unwrap());
        });
    });

    group.bench_with_input("parse-extended", TEST_RECIPE, |b, input| {
        let mut parser = parser();
        b.iter(|| {
            black_box(parser.parse(black_box(input), None).unwrap());
        });
    });

    group.bench_with_input("tokens-canonical", TEST_RECIPE, |b, input| {
        let mut parser = parser();
        b.iter(|| {
            let tree = parser.parse(black_box(input), None).unwrap();
            black_box(tree.root_node().descendant_count());
        });
    });

    group.bench_with_input("tokens-extended", TEST_RECIPE, |b, input| {
        let mut parser = parser();
        b.iter(|| {
            let tree = parser.parse(black_box(input), None).unwrap();
            black_box(tree.root_node().descendant_count());
        });
    });

    group.bench_with_input("meta", TEST_RECIPE, |b, input| {
        let mut parser = parser();
        b.iter(|| {
            black_box(parser.parse(black_box(input), None).unwrap());
        });
    });

    group.bench_with_input("frontmatter_meta", FRONTMATTER_RECIPE, |b, input| {
        let mut parser = parser();
        b.iter(|| {
            black_box(parser.parse(black_box(input), None).unwrap());
        });
    });
}

fn extended(c: &mut Criterion) {
    let mut group = c.benchmark_group("extended");

    group.bench_with_input("parse", COMPLEX_TEST_RECIPE, |b, input| {
        let mut parser = parser();
        b.iter(|| {
            black_box(parser.parse(black_box(input), None).unwrap());
        });
    });

    group.bench_with_input("tokens", COMPLEX_TEST_RECIPE, |b, input| {
        let mut parser = parser();
        b.iter(|| {
            let tree = parser.parse(black_box(input), None).unwrap();
            black_box(tree.root_node().descendant_count());
        });
    });
}

criterion_group!(benches, canonical, extended);
criterion_main!(benches);
