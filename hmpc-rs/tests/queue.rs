use std::collections::BTreeSet;
use std::iter::once;
use std::mem::size_of;
use std::ptr::NonNull;

use hmpc_rs::net::metadata::{AllGather, Broadcast, Gather};
use hmpc_rs::net::{Config, MessageSize, Queue};

static CONFIG: &str = "tests/mpc.yaml";

fn config() -> Config
{
    Config::read_env(Some(CONFIG.into())).unwrap()
}

trait AsNonNullPtrExt<T>
{
    fn as_non_null_ptr(&mut self) -> NonNull<T>;
}

impl<T> AsNonNullPtrExt<T> for Vec<T>
{
    /// # Safety
    /// Note that the pointer might be dangling, even if it is non-null
    fn as_non_null_ptr(&mut self) -> NonNull<T>
    {
        // SAFETY: `Vec::as_mut_ptr` is marked with `rustc_never_returns_null_ptr`
        unsafe { NonNull::new_unchecked(self.as_mut_ptr()) }
    }
}

#[test]
fn broadcast()
{
    const COUNT: usize = 5;
    const PARTIES: usize = 5;

    let communicator: BTreeSet<_> = [0, 1, 2, 3].into();
    assert_eq!(communicator.len() + 1, PARTIES);

    let mut queues: Vec<_> = communicator.iter()
        .chain(once(&4))
        .map(|&id| Queue::new(id, config()).unwrap())
        .collect();
    assert_eq!(queues.len(), PARTIES);

    let mut data = vec![vec![0u32; COUNT]; PARTIES];
    // Party 4 sends data
    data[4][0] = 4;
    data[4][1] = 1;
    data[4][2] = 2;
    data[4][3] = 3;
    data[4][4] = 4;

    let message = Broadcast::new(
        1,
        4,
        (size_of::<u32>() * COUNT) as MessageSize
    );

    let mut queues = queues.iter_mut();

    let results = tokio::runtime::Builder::new_current_thread().build().unwrap().block_on(async
    {
        tokio::join!(
            queues.next().unwrap().broadcast(message, &communicator, data[0].as_non_null_ptr().cast()),
            queues.next().unwrap().broadcast(message, &communicator, data[1].as_non_null_ptr().cast()),
            queues.next().unwrap().broadcast(message, &communicator, data[2].as_non_null_ptr().cast()),
            queues.next().unwrap().broadcast(message, &communicator, data[3].as_non_null_ptr().cast()),
            queues.next().unwrap().broadcast(message, &communicator, data[4].as_non_null_ptr().cast())
        )
    });

    results.0.unwrap();
    results.1.unwrap();
    results.2.unwrap();
    results.3.unwrap();
    results.4.unwrap();

    for data in data
    {
        assert_eq!(data.len(), COUNT);
        assert_eq!(data[0], 4);
        assert_eq!(data[1], 1);
        assert_eq!(data[2], 2);
        assert_eq!(data[3], 3);
        assert_eq!(data[4], 4);
    }
}

#[test]
fn gather()
{
    const COUNT: usize = 5;
    const PARTIES: usize = 5;

    let communicator: BTreeSet<_> = [10, 11, 12, 13].into();
    assert_eq!(communicator.len() + 1, PARTIES);

    let mut queues: Vec<_> = communicator.iter()
        .chain(once(&14))
        .map(|&id| Queue::new(id, config()).unwrap())
        .collect();
    assert_eq!(queues.len(), PARTIES);

    let mut data = vec![vec![0u32; COUNT]; PARTIES - 1];
    // Last party receives data
    for (i, data) in data.iter_mut().enumerate()
    {
        data[0] = i as u32;
        data[1] = 1;
        data[2] = 2;
        data[3] = 3;
        data[4] = 3;
    }

    let message = Gather::new(
        1,
        14,
        (size_of::<u32>() * COUNT) as MessageSize
    );

    let mut queues = queues.iter_mut();

    let mut result = vec![vec![0u32; COUNT]; PARTIES - 1];

    let results = tokio::runtime::Builder::new_current_thread().build().unwrap().block_on(async
    {
        let pointers = vec![
            vec![data[0].as_non_null_ptr().cast()],
            vec![data[1].as_non_null_ptr().cast()],
            vec![data[2].as_non_null_ptr().cast()],
            vec![data[3].as_non_null_ptr().cast()],
            vec![result[0].as_non_null_ptr().cast(), result[1].as_non_null_ptr().cast(), result[2].as_non_null_ptr().cast(), result[3].as_non_null_ptr().cast()],
        ];

        let mut pointers = pointers.into_iter();

        tokio::join!(
            queues.next().unwrap().gather(message, &communicator, pointers.next().unwrap()),
            queues.next().unwrap().gather(message, &communicator, pointers.next().unwrap()),
            queues.next().unwrap().gather(message, &communicator, pointers.next().unwrap()),
            queues.next().unwrap().gather(message, &communicator, pointers.next().unwrap()),
            queues.next().unwrap().gather(message, &communicator, pointers.next().unwrap())
        )
    });

    results.0.unwrap();
    results.1.unwrap();
    results.2.unwrap();
    results.3.unwrap();
    results.4.unwrap();

    assert_eq!(result.len(), PARTIES - 1);
    for (i, data) in result.iter().enumerate()
    {
        assert_eq!(data.len(), COUNT);
        assert_eq!(data[0], i as u32);
        assert_eq!(data[1], 1);
        assert_eq!(data[2], 2);
        assert_eq!(data[3], 3);
        assert_eq!(data[4], 3);
    }
}

#[test]
fn all_gather()
{
    const COUNT: usize = 6;
    const PARTIES: usize = 4;

    let communicator: BTreeSet<_> = [20, 21, 22, 23].into();
    assert_eq!(communicator.len(), PARTIES);

    let mut queues: Vec<_> = communicator.iter()
        .map(|&id| Queue::new(id, config()).unwrap())
        .collect();
    assert_eq!(queues.len(), PARTIES);


    let mut data = vec![vec![vec![0u32; COUNT]; PARTIES]; PARTIES];
    for (i, data) in data.iter_mut().enumerate()
    {
        data[i][0] = i as u32;
        data[i][1] = 1;
        data[i][2] = 2;
        data[i][3] = i as u32;
        data[i][4] = 3;
        data[i][5] = 4;
    }

    let message = AllGather::new(
        1,
        (size_of::<u32>() * COUNT) as MessageSize
    );

    let mut queues = queues.iter_mut();

    let results = tokio::runtime::Builder::new_current_thread().build().unwrap().block_on(async
    {
        let pointers = vec![
            vec![data[0][0].as_non_null_ptr().cast(), data[0][1].as_non_null_ptr().cast(), data[0][2].as_non_null_ptr().cast(), data[0][3].as_non_null_ptr().cast()],
            vec![data[1][0].as_non_null_ptr().cast(), data[1][1].as_non_null_ptr().cast(), data[1][2].as_non_null_ptr().cast(), data[1][3].as_non_null_ptr().cast()],
            vec![data[2][0].as_non_null_ptr().cast(), data[2][1].as_non_null_ptr().cast(), data[2][2].as_non_null_ptr().cast(), data[2][3].as_non_null_ptr().cast()],
            vec![data[3][0].as_non_null_ptr().cast(), data[3][1].as_non_null_ptr().cast(), data[3][2].as_non_null_ptr().cast(), data[3][3].as_non_null_ptr().cast()]
        ];

        let mut pointers = pointers.into_iter();

        tokio::join!(
            queues.next().unwrap().all_gather(message, &communicator, pointers.next().unwrap()),
            queues.next().unwrap().all_gather(message, &communicator, pointers.next().unwrap()),
            queues.next().unwrap().all_gather(message, &communicator, pointers.next().unwrap()),
            queues.next().unwrap().all_gather(message, &communicator, pointers.next().unwrap()),
        )
    });

    results.0.unwrap();
    results.1.unwrap();
    results.2.unwrap();
    results.3.unwrap();

    for data in data
    {
        assert_eq!(data.len(), PARTIES);
        for (i, data) in data.iter().enumerate()
        {
            assert_eq!(data.len(), COUNT);
            assert_eq!(data[0], i as u32);
            assert_eq!(data[1], 1);
            assert_eq!(data[2], 2);
            assert_eq!(data[3], i as u32);
            assert_eq!(data[4], 3);
            assert_eq!(data[5], 4);
        }
    }
}

#[test]
fn all_to_all()
{
    // TODO
}
